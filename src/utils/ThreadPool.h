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

namespace storage {
using namespace std;

enum class TaskStatus : Byte {
  UNINIT = 0, // Before initialize
  STARTED,    // The task has been added into ThreadPool, only for special task
  RUNNING,    // The task is running, it will be saved into thread local vector
  INTERVAL,   // The unfinished tasks has be moved from thread runnning queue to
              // pool queue and waitting free threads to catch them..
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

static inline BusyDegree GetBusyDegree(DT_MicroSec ts) {
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

public:
  virtual ~ThreadTask() {}
  virtual TaskStatus Run() = 0;
  // inline TaskStatus Status() { return _status; }
  // inline void SetStatus(TaskStatus s) { _status = s; }
  inline BusyDegree GetBusyDegree() { return _busyDegree; }
  inline uint16_t GetRepeatTime() { return _repeatTime; }

  // To occupy a thread entirely or not
  bool IsExclusiveTask() { return _bExclusive; };
  // Delete this task or not after this task has finished
  virtual bool IsNeedDelete() { return false; }

protected:
  // TaskStatus _status = TaskStatus::UNINIT;
  BusyDegree _busyDegree = BusyDegree::FREE;
  uint16_t _repeatTime{0}; // The same busy degree repeat time
  bool _bExclusive{false}; // To occupy a thread entirely or not

  // The count of current exclusive tasks,it must less than _maxThreads in
  // thread pool
  static atomic_uint32_t _exclusiveTasksCount;
};

struct ThreadPara {
  thread *_thread{nullptr};
  // The periodic tasks are run in this thread
  MVector<ThreadTask *> _vctTask;
  uint16_t _repeatTime{0}; // The same busy degree repeat time
  uint16_t _id;            // Thread id
  BusyDegree _busyDegree = BusyDegree::FREE;
  bool _bRunning{false};
  bool _bExclusiveTask{false}; // The running task is exclusive
};

class ThreadPool {
public:
  static inline string GetThreadName() { return _threadName; }
  static inline int GetThreadId() { return _threadID; }
  static void SetStop() { _stopThreads.store(true, memory_order_relaxed); }
  static bool IsStoped() { return _stopThreads.load(memory_order_relaxed); }

public:
  ThreadPool(string threadPrefix, int minThreads = 1,
             int maxThreads = DEFAULT_MAX_THREADS);
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void AddTask(ThreadTask *task);
  void AddTasks(MVector<ThreadTask *> &vct);
  void CreateThread(int id = -1);

  uint32_t GetTaskCount() { return (uint32_t)(_queueTask.size()); }

  uint32_t GetAliveThreadCount() const { return _aliveThreads; }
  uint32_t GetMinThreads() const { return _minThreads; }
  uint32_t GetMaxThreads() const { return _maxThreads; }

protected:
  // Check if the thread pool is busy or not, it will create new threads if
  // need.
  void ManageThreadPool();

protected:
  string _threadPrefix;
  int32_t _minThreads;
  int32_t _maxThreads;
  int32_t _aliveThreads{0};

  vector<ThreadPara> _vctThreadPara;
  SpinMutex _task_mutex;
  SpinMutex _threadMutex;
  condition_variable_any _taskCv;
  // To save new added tasks from outside
  MDeque<ThreadTask *> _queueTask;
  // For new added tasks. If ther has waitting tasks in _queueNewTask, below is
  // the last time to pop tasks from the _queueNewTask, or the time to add the
  // first tasks in _queueNewTask.
  DT_MicroSec _taskTime;
  // The last time to check if this thread pool is busy or not.
  atomic<DT_MicroSec> _checkBusyTime;
  // The busy status checked at last time.
  BusyDegree _poolBusyDegree;

protected:
  static atomic_bool _stopThreads;
  static thread_local int _threadID;
  static thread_local string _threadName;
};

} // namespace storage
