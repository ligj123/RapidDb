#pragma once

#include "../header.h"

#include "../cache/Mallocator.h"
#include "../core/IndexAction.h"
#include "../utils/ErrorMsg.h"
#include "../utils/RapidQueue.h"
#include "Session.h"

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
  // The session that have closed, they will be
  MVector<Session *> _discardSession;

  // To generate new TranID, every time it will add 1
  // Transaction ID is 64 bit unsigned integer. The highest 12 bit is node id
  // for distribute system, it can support max 4096 nodes. Following 4 bits is
  // cycle count of system start times, used to avoid transaction repeat. The
  // following 8 bits is used to save session group id. The last 40 bits is used
  // as auto increaseing counter.
  TranID _currTranId{0};
  // The tasks need to run
  vector<SessionTask *> _vctTask;
  // To receive the Actions from thread pool
  RapidQueue<ThreadAction> _threaPoolQueue;
  // To receive The actions from outside threads.
  RapidQueue<ThreadAction> _outsideQueue;
  // The task to run this group
  SessionTask *_task{nullptr};
};

class SessionTask : public ThreadTask {
public:
  SessionTask(MVector<SessionGroup *> &&vct) : _vctGroup(move(vct)) {}
  TaskStatus Run() override;
  void SetStop(bool b) { _bStop = true; }

protected:
  MVector<SessionGroup *> _vctGroup;
  bool _bStop{false};
};

class SessionPool {
public:
  static bool InitPool(uint16_t groupNum);

  static void ClosePool();

  static SessionGroup &GetSessionGroup(uint16_t gid) {
    assert(gid < _vctGroup.size());
    return _vctGroup[gid];
  }

  uint32_t CreateSession();
  void CloseSession(uint32_t sid);
  Session *GetSession(uint32_t sid);
  SessionGroup &GetSessionGroup(uint32_t sid) {
    return _vctGroup[sid % _threadNum];
  }
  // Apply a new transaction id. For efficiency, Every group will apply a range
  // of id pointed by TRAN_ID_RANGE, then every session will apply a transaction
  // from this range. If the ids in this range hasve been exhaust, it will apply
  // next range ids again.
  uint64_t ApplyTranId(uint32_t sid) {
    uint64_t &currTranId = _vctGroup[sid % _threadNum]._currTranId;
    uint64_t id = currTranId;
    currTranId++;

    if ((currTranId % TRAN_ID_RANGE) == 0) {
      currTranId = _tranId.fetch_add(TRAN_ID_RANGE, memory_order_relaxed);
      if (currTranId > (_tranInitId + (1LL << 40) - TRAN_ID_RANGE)) {
        unique_lock<SpinMutex> lock(_spinMutex);
        if (_tranId.load(memory_order_relaxed) >
            (_tranInitId + (1LL << 40) - TRAN_ID_RANGE)) {
          _tranId.store(_tranInitId, memory_order_relaxed);
          currTranId = _tranInitId;
        } else {
          currTranId = _tranId.fetch_add(TRAN_ID_RANGE, memory_order_relaxed);
        }
      }
    }

    return id;
  }

  static void AddAction(uint16_t poolId, uint16_t threadId, LeafRecord *lr) {
    assert(tid < _threadNum);
    _vctGroup[poolId]._recordQueue.Push(threadId, lr);
  }

  static RapidQueue<LeafRecord> &GetActionQueue(uint16_t poolId) {
    return _vctGroup[poolId]._recordQueue;
  }

protected:
  static void Run(uint16_t thdId);

protected:
  // The vector of session groups
  static MVector<SessionGroup> _vctGroup;

  static MVector<SessionTask *> _vctTask;
};
} // namespace storage