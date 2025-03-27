#pragma once

#include "../header.h"

#include "../cache/Mallocator.h"
#include "../utils/ErrorMsg.h"
#include "../utils/RapidQueue.h"
#include "Session.h"
#include "SessionAction.h"

#include <atomic>
#include <thread>

#define TRAN_ID_RANGE 0x1000

namespace storage {
using namespace std;
class SessionTask;

// In this pool, every thread has its data structor and it
// can only visit its data to avoid lock.
struct SessionGroup {
  SessionGroup(uint16_t groupSn, uint16_t nodeId, uint16_t restartNum,
               uint16_t poolThreadNum, uint16_t outsideThreadNum)
      : _groupSn(groupSn), _nodeId(nodeId), _restartNum(restartNum),
        _threaPoolQueue(poolThreadNum), _outerQueue(outsideThreadNum) {
    _currTranId = (((uint64_t)nodeId) << 52) + (((uint64_t)restartNum) << 48) +
                  (((uint64_t)groupSn) << 40);
  }

  SessionGroup(SessionGroup &&src)
      : _threaPoolQueue(move(src._threaPoolQueue)),
        _outerQueue(move(src._outerQueue)) {}
  ~SessionGroup() {
    assert(_lstAction.size() == 0);
    assert(_threaPoolQueue.RoughSize() == 0 && _outerQueue.RoughSize() == 0);
    for (auto iter = _mapSession.begin(); iter != _mapSession.end(); iter++) {
      assert(iter->second->_lstWaittingStmt.size() == 0 &&
             iter->second->_transaction.GetTranStatus() ==
                 TranStatus::FINISHED);
      delete iter->second;
    }

    _mapSession.clear();

    for (auto iter = _mapSqlExprStatement.begin();
         iter != _mapSqlExprStatement.end(); iter++) {
      delete iter->second;
    }

    _mapSqlExprStatement.clear();
    _mapIdExprStatement.clear();
  }

  // The session map that the sessions are alive.
  MHashMap<uint32_t, Session *> _mapSession;
  // To temp save the closed session, Until all its statement finished
  MVector<Session *> _obsoleteSession;

  uint16_t _groupSn;    // The serial number of this group
  uint16_t _nodeId;     // The id of this node, for distribute system
  uint16_t _restartNum; // The restart number of this system
  // To generate new TranID, every time it will add 1
  // Transaction ID is 64 bit unsigned integer. The highest 12 bit is node id
  // for distribute system, it can support max 4096 nodes. Following 4 bits is
  // cycle count of system start times, used to avoid transaction repeat. The
  // following 8 bits is used to save session group id. The last 40 bits is used
  // as auto increaseing counter.
  TranID _currTranId;
  // The actions need to run
  MList<SessionAction *> _lstAction;
  // To receive the Actions from thread pool
  RapidQueue<SessionAction> _threaPoolQueue;
  // To receive The actions from outside threads.
  RapidQueue<SessionAction> _outerQueue;
  // How many times this group has run. It only used for some actions do not run
  // too frequencly.
  uint64_t _runTimes{0};

  // The map of <Sql, parsed ExprStatement> in this session
  MStrHashMap<ExprStatement *> _mapSqlExprStatement;
  // The map of <exprstatement id, parsed ExprStatement> in this session,
  // duplicate of _mapSqlExprStatement.
  MHashMap<uint64_t, ExprStatement *> _mapIdExprStatement;
};

class SessionTask : public ThreadTask {
public:
  SessionTask(ThreadPool *threadPool, int sn) : ThreadTask(threadPool) {
    _taskName = "SessionTask" + ToMString(sn);
    _taskMask = UINT32_MAX;
  }
  SessionTask(ThreadPool *threadPool, int sn, MVector<SessionGroup *> &&vct)
      : ThreadTask(threadPool), _vctGroup(move(vct)) {
    _taskName = "SessionTask" + ToMString(sn);
    _taskMask = UINT32_MAX;
  }
  TaskStatus Run() override;
  void SetStop() { _bStop = true; }

  void AddSessionGroup(SessionGroup *group) { _vctGroup.push_back(group); }

  MVector<SessionGroup *> &GetVctSessionGroup() { return _vctGroup; }

protected:
  MVector<SessionGroup *> _vctGroup;
  bool _bStop{false};
  int _tryStopTime{5};
};

class SessionAdjustTask : public ThreadTask {
public:
  SessionAdjustTask(ThreadPool *threadPool, uint16_t newTaskNum)
      : ThreadTask(threadPool), _newTaskNum(newTaskNum) {
    _taskName = "SessionAdjustTask";
  }
  TaskStatus Run() override;
  bool IsNeedDelete() const override { return true; }

protected:
  uint16_t _newTaskNum;
  vector<SessionTask *> _vctOldTask;
};

class SessionPool {
public:
  static bool InitPool(uint16_t groupNum, uint16_t taskNum, uint16_t restartNum,
                       uint16_t userThreadNum, ThreadPool *threadPool,
                       bool bExclusive = false);

  static uint32_t CreateSession(uint16_t outerTid, StmtResult *result);
  static void CloseSession(uint16_t outerTid, uint32_t sessionId,
                           StmtResult *result);
  static void ClosePool() {
    for (SessionTask *task : _vctTask) {
      task->SetStop();
    }

    _bStop.store(true, memory_order_release);
  }

  static void ClearPool() {
    for (SessionTask *task : _vctTask) {
      assert(task->GetStatus(true) == TaskStatus::FINISHED);
      delete task;
    }

    _vctTask.clear();
    _vctGroup.clear();
    _threadPool = nullptr;
    _currSessionId.store(0, memory_order_relaxed);
  }

  static vector<SessionGroup> &GetVctSessionGroup() { return _vctGroup; }

  static Session *GetSession(uint32_t sid) {
    uint32_t gid = sid % (uint32_t)_vctGroup.size();
    auto iter = _vctGroup[gid]._mapSession.find(sid);
    assert(iter != _vctGroup[gid]._mapSession.end());
    return iter->second;
  }
  /**
   * @brief Add a SQL string as a new statement
   * @param outerTid Thread id that start from 0, it is NOT pool thread.
   * @param sid session id
   * @param stmtId statement id.Every session has independent statement id,
   * start from 0, every new request add 1, recycle if exceed UINT32_MAX. The
   * client will maintain it.
   * @param exprId The expression id, Every session has independent expression
   * id, start from 0, every new sql add 1, recycle if exceed UINT32_MAX. The
   * client will maintain it.
   * @param paras The multi lines of parameters,support batch rows.
   * @param result To save the result of executing the statement
   */
  static void AddStatement(uint16_t outerTid, uint32_t sid, uint32_t stmtId,
                           uint32_t exprId, MString &&sql, VectorRow &&paras,
                           StmtResult *result);
  /**
   * @brief Add a group of statements into SessionPool
   * @param mapAct key=16 bits SessionGroupId + 16 bits out thread id
   */
  static void
  AddStatements(MTreeMap<uint32_t, MVector<SessionStatementAction *>> &mapAct);
  /**
   * @brief Add action into SessionPool that the action come from this thread
   * pool
   * @param threadId The thread id in this thread pool
   * @param tranId Transaction id, used to calc session group id
   * @param action The action to add
   */
  static void AddAction(uint16_t threadId, TranID tranId,
                        SessionAction *action) {
    uint64_t gid = ((tranId >> 40) & 0xFF);
    assert(gid < _vctGroup.size());
    _vctGroup[gid]._threaPoolQueue.Push(threadId, action);
  }

  /**
   * @brief Add action into SessionPool from user threads.
   * @param outId Thread id that start from 0, it is NOT pool thread.
   * @param sid session id
   * @param action The action to add
   * @param result To save the result of executing the statement
   */
  static void AddAction(uint16_t outId, uint32_t sid, SessionAction *action,
                        StmtResult *result);

  static bool IsPoolStop() { return _bStop.load(memory_order_relaxed); }
  static vector<SessionTask *> &GetVctSessionTask() { return _vctTask; }
  // Generate a session id, only for testcase
  static uint32_t GenSessionId() {
    return _currSessionId.fetch_add(1, memory_order_relaxed);
  }

protected:
  // The vector of session groups
  static vector<SessionGroup> _vctGroup;
  // The vector of SessionTasks
  static vector<SessionTask *> _vctTask;
  // The ThreadPool to run tasks.
  static ThreadPool *_threadPool;
  // The system has stoped or not
  static atomic_bool _bStop;
  // To generate the session id, every time add 1
  static atomic_uint32_t _currSessionId;
};
} // namespace storage