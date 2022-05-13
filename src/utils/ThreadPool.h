﻿#pragma once

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

// All tasks that run in thread pool must inherit this class.
class Task {
public:
  virtual ~Task() {}
  virtual void Run() = 0;
  int GetResult() { return _result; }
  // If small task, the thread pool maybe get more than one tasks one time to
  // execute
  virtual bool IsSmallTask() { return true; }
  void IncRef(int num = 1, bool lock = false) {
    assert(num > 0);
    if (lock) {
      _spinMutex.lock();
    }
    _refCount += num;
    if (lock) {
      _spinMutex.unlock();
    }
  }
  void DecRef(int num = 1, bool lock = false) {
    assert(num > 0);
    if (lock) {
      _spinMutex.lock();
    }
    _refCount -= num;
    if (lock) {
      _spinMutex.unlock();
    }
    assert(_refCount >= 0);
    if (_refCount == 0) {
      delete this;
    }
  }

  bool AddWaitTasks(Task *task) { _vctWaitTasks.push_back(task); }

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /**Waiting tasks for this task*/
  vector<Task *> _vctWaitTasks;
  SpinMutex _spinMutex;
  uint32_t _refCount = 1;
  // 0: passed; -1: failed; 1: pause and resume in following time
  int _result;
};

class ThreadPool {
public:
  static thread_local int _threadID;
  static thread_local string _threadName;
  static string GetThreadName() { return _threadName; }
  static ThreadPool &InstMain() {
    if (_instMain == nullptr) {
      unique_lock<SpinMutex> lock(_smMain);
      if (_instMain == nullptr) {
        _instMain = new ThreadPool("Rapid");
      }
    }

    return *_instMain;
  }

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
  void CreateThread(int id = -1);
  int GetThreadCount() { return _aliveThreads; }
  int GetMinThreads() { return _minThreads; }
  int GetMaxThreads() { return _maxThreads; }
  int GetFreeThreads() { return _freeThreads; }

protected:
  vector<thread *> _vctThread;
  deque<Task *> _tasks;
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

protected:
  static ThreadPool *_instMain;
  static SpinMutex _smMain;
};

} // namespace storage
