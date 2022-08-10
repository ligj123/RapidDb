#pragma once

#include "../cache/Mallocator.h"
#include "ErrorMsg.h"
#include "SpinMutex.h"
#include <condition_variable>
#include <deque>
#include <future>
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
  STOPED             // The task has been remove from TimerThread
};

// All tasks that run in thread pool must inherit this class.
class Task {
public:
  virtual ~Task() {}
  virtual void Run() = 0;
  inline TaskStatus Status() { return _status; }
  // If small task, the thread pool maybe get more than one tasks one time to
  // execute
  virtual bool IsSmallTask() { return true; }
  // Only support add task before running
  void AddWaitTasks(Task *task) {
    assert(_status == TaskStatus::UNINIT);
    _vctWaitTasks.push_back(task);
  }

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /**Waiting tasks for this task*/
  MVector<Task *>::Type _vctWaitTasks;
  TaskStatus _status = TaskStatus::UNINIT;
};

class ThreadPool {
public:
  static thread_local int _threadID;
  static thread_local string _threadName;
  static thread_local Task *_currTask;
  static string GetThreadName() { return _threadName; }
  static ThreadPool &InstMain() {
    assert(_instMain != nullptr);
    return *_instMain;
  }
  static ThreadPool *
  InitMain(uint32_t maxQueueSize = 1000000, int minThreads = 1,
           int maxThreads = std::thread::hardware_concurrency()) {
    assert(_instMain == nullptr);
    unique_lock<SpinMutex> lock(_smMain);
    _instMain =
        new ThreadPool("RapidMain", maxQueueSize, minThreads, maxThreads);
    return _instMain;
  }

  void StopMain() {
    _instMain->Stop();
    delete _instMain;
    _instMain = nullptr;
  }

public:
  ThreadPool(string threadPrefix, uint32_t maxQueueSize = 1000000,
             int minThreads = 1,
             int maxThreads = std::thread::hardware_concurrency());
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void AddTask(Task *task, bool urgent = false);
  void AddTasks(MVector<Task *>::Type &vct);
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
  bool _stopThreads = false;
  string _threadPrefix;
  uint32_t _maxQueueSize;
  int _minThreads;
  int _maxThreads;
  int _aliveThreads;
  int _freeThreads;
  int _tasksNum;

protected:
  static ThreadPool *_instMain;
  static SpinMutex _smMain;
};

} // namespace storage
