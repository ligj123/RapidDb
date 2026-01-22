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
    while (!_bRemovedPool.load(memory_order_relaxed)) {
      this_thread::yield();
    }

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
  inline double GetAvgUsedTime() { return _dtUsed; }
  inline void SetAvgUsedTime(double us) { _dtUsed = us; }

  // To occupy a thread entirely or not
  bool IsExclusiveTask() { return _bExclusive; };
  void SetExclusiveTask(bool b) {
    if (b == _bExclusive) {
      return;
    }

    if (b) {
      _exclusiveTasksCount.fetch_add(1, memory_order_relaxed);
    } else {
      _exclusiveTasksCount.fetch_sub(1, memory_order_relaxed);
    }

    _bExclusive = b;
  }
  // Delete this task or not after this task has finished
  virtual bool IsNeedDelete() const { return false; }
  inline uint32_t GetTaskMask() const { return _taskMask; }
  inline const MString &GetTaskName() const { return _taskName; }
  inline void SetTaskName(MString &&name) { _taskName = move(name); }
  inline void SetRemovedPool(bool b) {
    _bRemovedPool.store(b, memory_order_relaxed);
  }
  inline bool IsRemovedPool() {
    return _bRemovedPool.load(memory_order_relaxed);
  }

protected:
  ThreadPool *_threadPool;
  // The average used time to run this task one time.
  // Equation = (current used us + previous used us * 99) / 100
  double _dtUsed{0};
  bool _bExclusive{false}; // To occupy a thread entirely or not
  atomic<TaskStatus> _taskStatus{TaskStatus::UNINIT};
  // If the tasks has same mask, they will try to avoid to hand out them into
  // one thread. If equal 0, means it does not to consider it.
  uint32_t _taskMask{0};
  // The count of current exclusive tasks,it must less than _maxThreads in
  // thread pool

  // Removed from ThreadPool or not
  atomic_bool _bRemovedPool{false};
  // Task name
  MString _taskName;

  static atomic_uint32_t _exclusiveTasksCount;
};

struct ThreadPara {
  ThreadPara() {}
  ThreadPara(ThreadPara &&src) {}

  bool IsMaskConflict(uint32_t mask) {
    return _mapTaskMask.find(mask) != _mapTaskMask.end();
  }

  void AddMask(uint32_t mask) {
    if (mask == 0) {
      return;
    }

    auto iter = _mapTaskMask.emplace(mask, 1);
    if (iter.second) {
      iter.first->second++;
    }
  }

  void RemoveMask(uint32_t mask) {
    auto iter = _mapTaskMask.find(mask);
    if (iter == _mapTaskMask.end()) {
      return;
    }

    iter->second--;
    if (iter->second == 0) {
      _mapTaskMask.erase(iter);
    }
  }

  void ClearMask() { _mapTaskMask.clear(); }

  thread *_thread{nullptr};
  // The periodic tasks are run in this thread
  MVector<ThreadTask *> _vctTask;
  // To receive task from manage thread of Pool
  LineQueue<ThreadTask> _lineQueueTask;
  // The average used time to run those task in this thread.
  // Equation = (current used us + previous used us * 99) / 100
  double _dtUsed{0};
  uint16_t _id; // Thread id in ThreadPool
  atomic_bool _bRunning{false};
  bool _bStop{true};
  bool _bExclusiveTask{false}; // The running task is exclusive
  bool _bSelRmTask{false};     // Selected to remove a task due to busy
  MHashMap<uint32_t, int> _mapTaskMask;

  double _dtUsedTotal{0}; // Total microSec used to run tasks.
  int64_t _runTimes{0};
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
  static ThreadPool *GetMainPool() { return _instMain; }

  static ThreadPool *CreateMainPool(const MString &threadPrefix = "main",
                                    int minThreads = 1,
                                    int maxThreads = DEFAULT_MAX_THREADS);
  static void CloseMainPool(bool ignoreTasks = false);

  static DT_MicroSec GetNow() { return _nowMicroSec; }

  static void PrintThreadTime();

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
      _rapidTaskQueue.Push(tid, task, false);
    }

    _rapidTaskQueue.Submit(tid);
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

    vct.clear();
  }

  uint32_t GetTaskCount() { return (uint32_t)(_queueTask.size()); }

  uint32_t GetAliveThreadCount() const { return _aliveThreads; }
  uint32_t GetMinThreads() const { return _minThreads; }
  uint32_t GetMaxThreads() const { return _maxThreads; }
  // Only for test
  void ClearTasks() {
    _rapidTaskQueue.Pop(_queueTask);
    for (ThreadTask *task : _queueTask) {
      task->SetStatus(TaskStatus::FINISHED, true);
      if (task->IsNeedDelete()) {
        delete task;
      }
    }

    _queueTask.clear();
  }

  void WaitStoped() {
    if (_threadMgr == nullptr) {
      return;
    }

    _threadMgr->join();
    delete _threadMgr;
    _threadMgr = nullptr;
  }

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
  // The threads' parameters in this pool
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
  MList<ThreadTask *> _queueTask;
  // The last time to check the busy status of the thread pool.
  DT_MicroSec _checkBusyTime;
  // The busy status of this thread pool
  BusyDegree _poolBusyDegree{BusyDegree::RELAXED};

protected:
  static ThreadPool *_instMain;
  static atomic_bool _stopThreads;
  static thread_local int _threadID;
  static thread_local MString _threadName;
  // The current datetime in micro second
  static DT_MicroSec _nowMicroSec;
};

} // namespace storage
