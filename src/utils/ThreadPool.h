#pragma once

#include "../cache/Mallocator.h"
#include "RapidQueue.h"
#include "SpinMutex.h"

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
  INTERVAL,   // The periodic task has not new tasks long time and will be moved
              // into thread pool queue.
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
  } else if (ts < 300) {
    return BusyDegree::RELAXED;
  } else if (ts < 10000) {
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
  inline TaskStatus Status() { return _status; }
  inline void SetStatus(TaskStatus s) { _status = s; }
  inline BusyDegree GetBusyDegree() { return _busyDegree; }
  inline uint16_t GetRepeatTime() { return _repeatTime; }

  // If true, the task will rerun again after current time until its status is
  // finish
  virtual bool IsPeriodicTask() { return true; };
  // Free this task or not after this task has finished
  virtual bool IsNeedFree() { return false; }

protected:
  TaskStatus _status = TaskStatus::UNINIT;
  BusyDegree _busyDegree = BusyDegree::FREE;
  uint16_t _repeatTime{0}; // The same busy degree repeat time
};

struct ThreadPara {
  thread _thread;
  // The periodic tasks are run in this thread
  MVector<ThreadTask *> _vctTask;
  BusyDegree _busyDegree = BusyDegree::FREE;
  uint16_t _repeatTime{0}; // The same busy degree repeat time
  bool _bRunning{false};
};

class ThreadPool {
public:
  static inline string GetThreadName() { return _threadName; }
  static inline int GetThreadId() { return _threadID; }

public:
  ThreadPool(string threadPrefix, uint32_t maxQueueSize = 1000000,
             int minThreads = 1, int maxThreads = DEFAULT_MAX_THREADS);
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void AddTask(ThreadTask *task);
  void AddTasks(MVector<ThreadTask *> &vct);
  void CreateThread(int id = -1);

  bool IsFull() { return _queueTask.size() > _maxQueueSize; }

  void SetStop() { _stopThreads = true; }
  uint32_t GetTaskCount() { return (uint32_t)(_queueTask.size()); }
  void SetMaxQueueSize(uint32_t qsize) { _maxQueueSize = qsize; }
  uint32_t GetMaxQueueSize() { return _maxQueueSize; }
  uint32_t GetAliveThreadCount() const { return _aliveThreads; }
  uint32_t GetMinThreads() const { return _minThreads; }
  uint32_t GetMaxThreads() const { return _maxThreads; }

protected:
  string _threadPrefix;
  int32_t _maxQueueSize;
  int32_t _minThreads;
  int32_t _maxThreads;
  int32_t _aliveThreads{0};

  vector<ThreadPara> _vctThreadPara;
  SpinMutex _task_mutex;
  SpinMutex _threadMutex;
  condition_variable_any _taskCv;

  MDeque<ThreadTask *> _queueTask;
  // If ther has waitting tasks in ThreadPool, below is the last time one thread
  // to pop tasks from the vector, or the time to add the first tasks.
  DT_MicroSec _taskTime;
  bool _stopThreads{false};

protected:
  static thread_local int _threadID;
  static thread_local string _threadName;
};

} // namespace storage
