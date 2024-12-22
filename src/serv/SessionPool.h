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
  SessionGroup(uint16_t poolThreadNum, uint16_t outsideThreadNum)
      : _threaPoolQueue(poolThreadNum), _outerQueue(outsideThreadNum) {}
  SessionGroup(SessionGroup &&src)
      : _threaPoolQueue(move(src._threaPoolQueue)),
        _outerQueue(move(src._outerQueue)) {}
  ~SessionGroup() {
    assert(_lstAction.size() == 0);
    assert(_threaPoolQueue.RoughSize() == 0 && _outerQueue.RoughSize() == 0);
    for (auto iter = _mapSession.begin(); iter != _mapSession.end(); iter++) {
      assert(iter->second->_lstWaittingStmt.size() == 0 &&
             iter->second->_transaction.GetTranStatus() ==
                 TaskStatus::FINISHED);
      delete iter->second;
    }

    _mapSession.clear();
  }

  // The session map that the sessions are alive.
  MHashMap<uint32_t, Session *> _mapSession;
  // To temp save the closed session, Until all its statement finished
  MVector<Session *> _obsoleteSession;

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
  // The task to run this group
  SessionTask *_task{nullptr};
};

class SessionTask : public ThreadTask {
public:
  SessionTask(ThreadPool *threadPool) : ThreadTask(threadPool) {}
  SessionTask(ThreadPool *threadPool, MVector<SessionGroup *> &&vct)
      : ThreadTask(threadPool), _vctGroup(move(vct)) {}
  TaskStatus Run() override;
  void SetStop() { _bStop = true; }

  void AddSessionGroup(SessionGroup *group) { _vctGroup.push_back(group); }
  bool IsNeedDelete() override { return true; }

protected:
  MVector<SessionGroup *> _vctGroup;
  bool _bStop{false};
};

class SessionPool {
public:
  static void InitPool(uint16_t groupNum, uint16_t taskNum, uint16_t restartNum,
                       uint16_t outsiteThreadNum, ThreadPool *threadPool);
  static void AdjustTaskNumber(uint16_t newTaskNum);
  static uint32_t CreateSession(uint16_t outerTid, StmtResult *result);
  static void CloseSession(uint16_t outerTid, uint32_t sessionId,
                           StmtResult *result);
  static void ClosePool() {
    for (SessionTask *task : _vctTask) {
      task->SetStop();
    }

    _bStop.store(true, memory_order_release);
  }

  static MVector<SessionGroup> &GetVctSessionGroup() { return _vctGroup; }

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

protected:
  // The vector of session groups
  static MVector<SessionGroup> _vctGroup;
  // The vector of SessionTasks
  static MVector<SessionTask *> _vctTask;
  // The ThreadPool to run tasks.
  static ThreadPool *_threadPool;
  // The system has stoped or not
  static atomic_bool _bStop;
  // To generate the session id, every time add 1
  static atomic_uint32_t _currSessionId;
};
} // namespace storage