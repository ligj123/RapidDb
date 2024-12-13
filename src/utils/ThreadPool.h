#pragma once

#include "../cache/Mallocator.h"
#include "RapidQueue.h"
#include "SpinMutex.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <set>
#include <thread>
#include <type_traits>
#include <unordered_map>

#define MASK_SIZE 4
namespace storage {
using namespace std;
class ThreadPool;

enum class TaskStatus : Byte {
  UNINIT = 0, // Before initialize
  STARTED,    // The task has been added into ThreadPool, only for special task
  RUNNING,    // The task is running, it will be saved into thread local vector
  INTERVAL,   // The unfinished tasks has be moved from thread runnning queue to
              // pool queue and waitting free threads to catch them.
  FREE,       // There has not more tasks to run
  FINISHED    // The task has finished and will be free.
};

inline std::ostream &operator<<(std::ostream &os, const TaskStatus &sta) {
  switch (sta) {
  case TaskStatus::UNINIT:
    os << "UNINIT(" << (int)TaskStatus::UNINIT << ")";
    break;
  case TaskStatus::STARTED:
    os << "STARTED(" << (int)TaskStatus::STARTED << ")";
    break;
  case TaskStatus::RUNNING:
    os << "RUNNING(" << (int)TaskStatus::RUNNING << ")";
    break;
  case TaskStatus::INTERVAL:
    os << "INTERVAL(" << (int)TaskStatus::INTERVAL << ")";
    break;
  case TaskStatus::FINISHED:
    os << "FINISHED(" << (int)TaskStatus::FINISHED << ")";
    break;
  }

  return os;
}

enum class BusyDegree : Byte {
  EMPTY = 0, // It has not any task for ThreadTask in a round
  FREE,      // Less than 10us every round
  RELAXED,   // Between 10us and 300us every round
  BUSY,      // Between 300us and 10ms every round
  BLOCKED    // More than 10ms every round
};

static inline BusyDegree CalcBusyDegree(DT_MicroSec ts) {
  if (ts < 10) {
    return BusyDegree::FREE;
  } else if (ts < 100) {
    return BusyDegree::RELAXED;
  } else if (ts < 1000) {
    return BusyDegree::BUSY;
  } else {
    return BusyDegree::BLOCKED;
  }
}

// All tasks that run in thread pool must inherit this class.
class ThreadTask {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
  static uint32_t GetExclusiveTaskCount() {
    return _exclusiveTasksCount.load(memory_order_relaxed);
  }

public:
  ThreadTask(ThreadPool *threadPool) : _threadPool(threadPool) {}
  ThreadTask(const ThreadTask &src) = delete;
  ThreadTask(ThreadTask &&src) = delete;
  virtual ~ThreadTask() {
    if (_bExclusive) {
      SetExclusiveTask(false);
    }
  }
  ThreadTask &operator=(const ThreadTask &src) = delete;
  ThreadTask &operator=(ThreadTask &&src) = delete;

  virtual TaskStatus Run() = 0;
  inline TaskStatus GetStatus(bool acquire) {
    return _taskStatus.load(acquire ? memory_order_acquire
                                    : memory_order_relaxed);
  }
  inline void SetStatus(TaskStatus s, bool release) {
    _taskStatus.store(s, release ? memory_order_release : memory_order_relaxed);
  }
  // inline void SetStatus(TaskStatus s) { _status = s; }
  inline BusyDegree GetBusyDegree() { return _busyDegree; }
  inline uint8_t GetRepeatTime() { return _repeatTime; }

  // To occupy a thread entirely or not
  bool IsExclusiveTask() { return _bExclusive; };
  void SetExclusiveTask(bool b) {
    if (b && !_bExclusive) {
      _exclusiveTasksCount.fetch_add(1, memory_order_relaxed);
    } else if (!b && _bExclusive) {
      _exclusiveTasksCount.fetch_sub(1, memory_order_relaxed);
    }

    _bExclusive = b;
  }
  // Delete this task or not after this task has finished
  virtual bool IsNeedDelete() { return false; }
  virtual bool IsCycleTask() { return true; }
  inline uint32_t GetTaskMask() { return _taskMask; }

protected:
  ThreadPool *_threadPool;
  BusyDegree _busyDegree = BusyDegree::FREE;
  uint8_t _repeatTime{0};  // The same busy degree repeat time
  bool _bExclusive{false}; // To occupy a thread entirely or not
  atomic<TaskStatus> _taskStatus{TaskStatus::UNINIT};
  // If the tasks has same mask, they will try to avoid to hand out them into
  // one thread. If equal 0, means it does not to avoid it.
  uint32_t _taskMask{0};
  // The count of current exclusive tasks,it must less than _maxThreads in
  // thread pool
  static atomic_uint32_t _exclusiveTasksCount;
};

struct ThreadPara {
  ThreadPara() {}
  ThreadPara(ThreadPara &&src) {}

  bool IsMaskConflict(uint32_t mask) {
    if (mask == 0) {
      return false;
    }

    for (int i = 0; i < MASK_SIZE; i++) {
      if (_arrTaskMask[i] == mask) {
        return true;
      }
    }
    return false;
  }

  void SetMask(uint32_t mask) {
    if (mask == 0) {
      return;
    }

    int pos = -1;
    for (int i = 0; i < MASK_SIZE; i++) {
      if (_arrTaskMask[i] == mask) {
        return;
      } else if (_arrTaskMask[i] == 0) {
        pos = i;
      }
    }
    if (pos >= 0) {
      _arrTaskMask[pos] = mask;
    }
  }

  void ClearMask() {
    for (int i = 0; i < MASK_SIZE; i++) {
      _arrTaskMask[i] = 0;
    }
  }

  thread *_thread{nullptr};
  // The periodic tasks are run in this thread
  MVector<ThreadTask *> _vctTask;
  // To receive task from manage thread of Pool
  LineQueue<ThreadTask> _lineQueueTask;
  uint16_t _repeatTime{0}; // The same busy degree repeat time
  uint16_t _id;            // Thread id
  BusyDegree _busyDegree = BusyDegree::FREE;
  atomic_bool _bRunning{false};
  bool _bStop{true};
  bool _bExclusiveTask{false}; // The running task is exclusive
  // The last time to remove task from busy queue
  DT_MicroSec _dtRemoveTask{0};
  uint32_t _arrTaskMask[MASK_SIZE]{};
};

class ThreadPool {
public:
  static inline const MString &GetThreadName() { return _threadName; }
  static inline int GetThreadId() { return _threadID; }
  static void SetStop() { _stopThreads.store(true, memory_order_relaxed); }
  static bool IsStoped() { return _stopThreads.load(memory_order_relaxed); }
  // Set current thread id, only used for test
  static inline uint16_t SetThreadId(uint16_t id) {
    uint16_t tmp = _threadID;
    _threadID = id;
    return tmp;
  }
  static ThreadPool *GetMainPool() {
    assert(_instMain != nullptr);
    return _instMain;
  }

  static void CreateMainPool(const MString &threadPrefix = "main",
                             int minThreads = 1,
                             int maxThreads = DEFAULT_MAX_THREADS);
  static void CloseMainPool();

public:
  ThreadPool(const MString &threadPrefix, int minThreads = 1,
             int maxThreads = DEFAULT_MAX_THREADS);
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void AddTask(uint16_t tid, ThreadTask *task) {
    _rapidTaskQueue.Push(tid, task);
  }

  void AddTasks(uint16_t tid, MVector<ThreadTask *> &vct) {
    for (auto task : vct) {
      _rapidTaskQueue.Push(tid, task);
    }

    vct.clear();
  }

  void AddTask(ThreadTask *task) {
    std::unique_lock<SpinMutex> queue_lock(_taskMutex);
    _queueTask.push_back(task);
  }

  void AddTasks(MVector<ThreadTask *> &vct) {
    if (vct.size() == 0)
      return;

    std::unique_lock<SpinMutex> queue_lock(_taskMutex);
    for (auto task : vct) {
      _queueTask.push_back(task);
    }
  }

  uint32_t GetTaskCount() { return (uint32_t)(_queueTask.size()); }

  uint32_t GetAliveThreadCount() const { return _aliveThreads; }
  uint32_t GetMinThreads() const { return _minThreads; }
  uint32_t GetMaxThreads() const { return _maxThreads; }
  DT_MicroSec GetNow() const { return _nowMicroSec; }

protected:
  void CreateWorkThread(int id = -1);
  // Check if the thread pool is busy or not, it will create new threads if
  // need.
  void CheckBusyStatus();
  // The process for work thread
  void ManageProc();
  // The process for managing thread
  void WorkProc(uint16_t tid);

protected:
  MString _threadPrefix;
  int32_t _minThreads;
  int32_t _maxThreads;
  int32_t _aliveThreads{0};
  // The threads' parameters in this poll
  vector<ThreadPara> _vctThreadPara;
  // Receive IndexTask From the threads in this pool
  RapidQueue<ThreadTask> _rapidTaskQueue;
  // The managing thread of this pool. It will response create work threads,
  // collect work threads data, and decide if the work threads will stop and
  // hand out the IndexTasks to work threads.
  thread *_threadMgr{nullptr};
  // Only used for NON pool thread to add tasks.
  SpinMutex _taskMutex;
  // To accept new tasks from NON pool threads, it will use mutex to ensure data
  // consistency
  MDeque<ThreadTask *> _queueTask;
  // The last time to check the busy status of the thread pool.
  DT_MicroSec _checkBusyTime;
  // The busy status of this thread pool
  BusyDegree _poolBusyDegree{BusyDegree::RELAXED};
  // The current datetime in micro second
  DT_MicroSec _nowMicroSec;

protected:
  static ThreadPool *_instMain;
  static atomic_bool _stopThreads;
  static thread_local int _threadID;
  static thread_local MString _threadName;
};

} // namespace storage
