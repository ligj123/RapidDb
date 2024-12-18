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
  // The session map that the sessions are alive.
  MHashMap<uint32_t, Session *> _mapSession;

  // To generate new TranID, every time it will add 1
  // Transaction ID is 64 bit unsigned integer. The highest 12 bit is node id
  // for distribute system, it can support max 4096 nodes. Following 4 bits is
  // cycle count of system start times, used to avoid transaction repeat. The
  // following 8 bits is used to save session group id. The last 40 bits is used
  // as auto increaseing counter.
  TranID _currTranId;
  // The tasks need to run
  MVector<SessionAction *> _vctTask;
  // To receive the Actions from thread pool
  RapidQueue<SessionAction> _threaPoolQueue;
  // To receive The actions from outside threads.
  RapidQueue<SessionAction> _outsideQueue;
  // The task to run this group
  SessionTask *_task{nullptr};
};

class SessionTask : public ThreadTask {
public:
  SessionTask() {}
  SessionTask(MVector<SessionGroup *> &&vct) : _vctGroup(move(vct)) {}
  TaskStatus Run() override;
  void SetStop(bool b) { _bStop = true; }

  void AddSessionGroup(SessionGroup *group) { _vctGroup.push_back(group); }
  bool IsNeedDelete() override { return true; }

protected:
  MVector<SessionGroup *> _vctGroup;
  bool _bStop{false};
};

class SessionPool {
public:
  static bool InitPool(uint16_t groupNum, uint16_t taskNum, uint16_t startNum);

  static void ClosePool() {
    for (SessionTask *task : _vctTask.size()) {
      task->_bStop = true;
    }

    _bStop.store(true, memory_order_release);
  }

  static SessionGroup &GetSessionGroup(uint16_t gid) {
    assert(gid < _vctGroup.size());
    return _vctGroup[gid];
  }

  static uint32_t CreateSession(function<void()> hookFunc = nullptr);
  static void CloseSession(uint32_t sid);
  static Session *GetSession(uint32_t sid) {
    uint32_t gid = sid % (uint32_t)_vctGroup.size();
    auto iter = _vctGroup[gid]._mapSession.find(sid);
    assert(iter != _vctGroup[gid]._mapSession.end());
    return iter->second;
  }
  /**
   * @brief Add a SQL string as a new statement
   * @param tid Thread id that start from 0, it is NOT pool thread.
   * @param sid session id
   * @param stmtId statement id.Every session has independent statement id,
   * start from 0, every new request add 1, recycle if exceed UINT32_MAX. The
   * client will maintain it.
   * @param exprId The expression id, Every session has independent expression
   * id, start from 0, every new sql add 1, recycle if exceed UINT32_MAX. The
   * client will maintain it.
   * @param paras The multi lines of parameters,support batch rows.
   * @param result The result of executing the statement
   */
  static void AddStatement(uint16_t tid, uint32_t sid, uint32_t stmtId,
                           uint32_t exprId, MString &sql, VectorRow &&paras,
                           StmtResult &result);

  /**
   * @brief Add action into SessionPool that the action come from this thread
   * pool
   * @param threadId The thread id in this thread pool
   * @param tranId Transaction id, used to calc session group id
   * @param action The action to add
   */
  static void AddAction(uint16_t threadId, TranID tranId,
                        ThreadAction *action) {
    uint64_t gid = ((tranId >> 40) & 0xFF);
    assert(gid < _vctGroup.size());
    assert(threadId < ThreadPool::GetMaxThreads());
    _vctGroup[gid]._threaPoolQueue.Push(threadId, action);
  }

  static RapidQueue<LeafRecord> &GetActionQueue(uint16_t gid) {
    return _vctGroup[gid]._threaPoolQueue;
  }

protected:
  // The vector of session groups
  static MVector<SessionGroup> _vctGroup;

  static MVector<SessionTask *> _vctTask;

  atomic_bool _bStop{false};
};
} // namespace storage