#pragma once

#include "../cache/Mallocator.h"
#include "ErrorMsg.h"
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
  STARTED,    // The task has been added into TimerThread, only for special task
  RUNNING,    // The task is running
  INTERVAL,   // Interval time between two running
  PAUSE_WITHOUT_ADD, // Pause task and not need to add this task into ThreadPool
  PAUSE_WITH_ADD,    // Pause task and add this task into ThreadPool
  FINISHED           // The task has finished and will be free.
};

// All tasks that run in thread pool must inherit this class.
class Task {
public:
  virtual ~Task() {}
  virtual void Run() = 0;
  inline TaskStatus Status() { return _status; }
  inline void SetStatus(TaskStatus s) { _status = s; }
  // If small task, the thread pool maybe get more than one tasks one time to
  // execute
  virtual bool IsSmallTask() { return true; }
  // Only support add task before running
  void AddWaitTasks(Task *task) {
    assert(_status == TaskStatus::UNINIT);
    _vctWaitTasks.push_back(task);
  }

  MVector<Task *> &GetWaitTasks() { return _vctWaitTasks; }
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /**The tasks waiting for this task, they will be added into pool when current
   * has been finished*/
  MVector<Task *> _vctWaitTasks;
  TaskStatus _status = TaskStatus::UNINIT;
};

template <class T> class FastQueue;
class ThreadPool {
public:
  static thread_local Task *_currTask;
  static inline string GetThreadName() { return _threadName; }
  static inline int GetThreadId() { return _threadID; }
  static inline void AddThread(string name, int id) {
    _threadName = name;
    _threadID = id;
#ifdef DEBUG_TEST
    if (id >= 0) {
      if (_setId.find(id) != _setId.end())
        abort();
      _setId.insert(id);
    }
#endif
  }
  static inline void RemoveThread(int id) {
#ifdef DEBUG_TEST
    if (id >= 0)
      _setId.erase(id);
#endif
  }
  static ThreadPool &InstMain() {
    assert(_instMain != nullptr);
    return *_instMain;
  }
  static ThreadPool *InitMain(uint32_t maxQueueSize = 1000000,
                              int minThreads = 1,
                              int maxThreads = DEFAULT_MAX_THREADS,
                              int startId = 0) {
    assert(_instMain == nullptr);
    unique_lock<SpinMutex> lock(_smMain);
    _instMain = new ThreadPool("RapidMain", maxQueueSize, minThreads,
                               maxThreads, startId);
    return _instMain;
  }

  static void StopMain() {
    _instMain->Stop();
    delete _instMain;
    _instMain = nullptr;
  }

public:
  ThreadPool(string threadPrefix, uint32_t maxQueueSize = 1000000,
             int minThreads = 1, int maxThreads = DEFAULT_MAX_THREADS,
             int startId = 0);
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void AddTask(Task *task, bool urgent = false);
  void AddTasks(MVector<Task *> &vct);
  void Stop() { _stopThreads = true; }
  uint32_t GetTaskCount() {
    return (uint32_t)(_smallTasks.size() + _largeTasks.size() +
                      _urgentTasks.size());
  }
  void SetMaxQueueSize(uint32_t qsize) { _maxQueueSize = qsize; }
  uint32_t GetMaxQueueSize() { return _maxQueueSize; }
  bool IsFull() {
    return _maxQueueSize <=
           _smallTasks.size() + _largeTasks.size() + _urgentTasks.size();
  }
  void CreateThread(int id = -1);
  int GetThreadCount() { return _aliveThreads; }
  int GetMinThreads() { return _minThreads; }
  int GetMaxThreads() { return _maxThreads; }
  int GetFreeThreads() { return _freeThreads; }

protected:
  vector<thread *> _vctThread;
  deque<Task *> _smallTasks;
  deque<Task *> _largeTasks;
  deque<Task *> _urgentTasks;
  SpinMutex _task_mutex;
  SpinMutex _threadMutex;
  condition_variable_any _taskCv;
  string _threadPrefix;
  bool _stopThreads = false;
  uint32_t _maxQueueSize;
  int _minThreads;
  int _maxThreads;
  int _aliveThreads;
  int _freeThreads;
  int _tasksNum;
  // The start id for this pool, this parameter used for multi pool in one
  // pragram and the ids must be series between pools. Because FastQueue will
  // use those ids as index.
  int _startId;
  FastQueue<Task> *_fastQueue;

protected:
  static ThreadPool *_instMain;
  static SpinMutex _smMain;
  static thread_local int _threadID;
  static thread_local string _threadName;

#ifdef DEBUG_TEST
  // Sve have used thread id to avoid id collide
  static set<int> _setId;
#endif
};

} // namespace storage
