#pragma once

#include "../cache/Mallocator.h"
#include "ErrorMsg.h"
#include "SpinMutex.h"
#include <condition_variable>
#include <deque>
#include <future>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>

namespace storage {
using namespace std;

// All tasks need to be run in thread pool must inherit this class.
class Task {
public:
  virtual ~Task() {}
  virtual void Run() = 0;
  // All child class will return int after call Run, 0: passed; -1: Failed;
  // If need throw exception, it will throw ErrorMsg;
  future<int> GetFuture() { return _promise.get_future(); }
  // If small task, the thread pool maybe get more than one tasks one time to
  // execute
  virtual bool IsSmallTask() { return false; }


  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /**Return int value after finished.*/
  promise<int> _promise;
  /**Waiting tasks for this task*/
  vector<Task *> _vctWaitTasks;
};

class ThreadPool {
public:
  static thread_local string _threadName;
  static string GetThreadName() { return _threadName; }

public:
  ThreadPool(string threadPrefix, uint32_t maxQueueSize = 1000000,
             int minThreads = 1,
             int maxThreads = std::thread::hardware_concurrency());
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void AddTask(Task *task);
  void AddTasks(MVector<Task *>::Type &vct);
  void Stop() { _stopThreads = true; }
  uint32_t GetTaskCount() { return (uint32_t)_tasks.size(); }
  void SetMaxQueueSize(uint32_t qsize) { _maxQueueSize = qsize; }
  uint32_t GetMaxQueueSize() { return _maxQueueSize; }
  bool IsFull() { return _maxQueueSize <= _tasks.size(); }
  void CreateThread(int id);
  int GetThreadCount() { return (int)_mapThread.size(); }
  int GetMinThreads() { return _minThreads; }
  int GetMaxThreads() { return _maxThreads; }
  int GetCurrId() { return _currId; }
  int GetFreeThreads() { return _freeThreads; }

private:
  unordered_map<int, std::thread *> _mapThread;
  deque<Task *> _tasks;
  SpinMutex _task_mutex;
  SpinMutex _threadMutex;
  condition_variable_any _taskCv;
  bool _stopThreads = false;
  string _threadPrefix;
  uint32_t _maxQueueSize;
  int _minThreads;
  int _maxThreads;
  atomic_int32_t _currId;
  int _freeThreads;
};

} // namespace storage
