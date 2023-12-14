#include "../cache/Mallocator.h"
#include "Session.h"
#include <atomic>
#include <thread>

#define TRAN_ID_RANGE 0x1000

namespace storage {
using namespace std;

enum class STaskType : uint8_t {
  Create = 0, // Create a session
  Close       // Close a session
};

struct SessionTask {
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  virtual STaskType TaskType() const = 0;
  virtual void Exec() = 0;
  uint32_t _sid; // session id
};

struct CreateSession : public SessionTask {
  STaskType TaskType() override const { return STaskType::Create; }
  void Exec() override;
  Session *_session;
};

struct CloseSession : public SessionTask {
  STaskType TaskType() override const { return STaskType::Close; }
  void Exec() override;
};

// In this pool, every thread has its data structor and it
// can only visit its data to avoid lock.
struct SessionGroup {
  // The session map that the sessions are alive.
  MHashMap<uint32_t, Session *> _mapSession;
  // The session that have closed, they will be
  MVector<Session *> _discardSession;
  // The thread for current group
  thread *_thread{nullptr};
  // The current transaction id to assign
  uint64_t _currTranId{0};
  // The tasks need to run
  vector<SessionTask *> _vctTask;
};

class SessionPool {
public:
  static bool InitPool(uint16_t threadNum);

  static void ClosePool() {
    delete[] _arMapSession;
    delte[] _arThread;
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
      if (currTranId > (_tranInitId + (2 << 40) - TRAN_ID_RANGE)) {
        unique_lock<SpinMutex> lock(_spinMutex);
        if (_tranId.load(memory_order_relaxed) >
            (_tranInitId + (2 << 40) - TRAN_ID_RANGE)) {
          _tranId.store(_tranInitId, memory_order_relaxed);
          currTranId = _tranInitId;
        } else {
          currTranId = _tranId.fetch_add(TRAN_ID_RANGE, memory_order_relaxed);
        }
      }
    }

    return id;
  }

protected:
  static void Run(uint16_t thdId);

protected:
  static bool _bStoped;
  // Create how much threads to run session.
  static uint16_t _threadNum;
  // The vector of session groups
  static vector<SessionGroup> _vctGroup;
  // Every time to start db, it will start from 0, every group will get a range
  // of session id every time to avoid visit this atomic variable too much time.
  static atomic<uint64_t> _sessionId;
  // The first 16 bits was reserved for distributed db. Next 8 bits use to show
  // how many times to restart db, it will be saved in system variable table.
  // The other 40 bits is the session id incremented 1 every time.
  static atomic<uint64_t> _tranId;
  // The init transaction this time
  static uint64_t _tranInitId;
  static SpinMutex _spinMutex;
};
} // namespace storage