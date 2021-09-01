#pragma once

#include "SpinMutex.h"
#include <condition_variable>
#include <future>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>

namespace storage {
using namespace std;

class ThreadPool {
public:
  static thread_local string _threadName;
  static string GetThreadName() { return _threadName; }

public:
  ThreadPool(string threadPrefix, uint32_t maxQueueSize = 1000000,
             int minThreads = 1,
             int maxThreads = std::thread::hardware_concurrency());
  ~ThreadPool();

  // since std::thread objects are not copiable, it doesn't make sense for a
  //  thread_pool to be copiable.
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  template <typename F, typename... Args> auto AddTask(F &&, Args &&...);
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
  class task_container_base {
  public:
    virtual ~task_container_base(){};
    virtual void operator()() = 0;
  };
  using task_ptr = std::unique_ptr<task_container_base>;

  template <typename F> class task_container : public task_container_base {
  public:
    task_container(F &&func) : _f(std::forward<F>(func)) {}
    void operator()() override { _f(); }

  private:
    F _f;
  };

  unordered_map<int, std::thread *> _mapThread;
  queue<task_ptr> _tasks;
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

template <typename F, typename... Args>
auto ThreadPool::AddTask(F &&function, Args &&...args) {
  std::unique_lock<SpinMutex> queue_lock(_task_mutex, std::defer_lock);
  std::packaged_task<std::invoke_result_t<F, Args...>()> task_pkg(
      [_f = std::move(function),
       _fargs = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        return std::apply(std::move(_f), std::move(_fargs));
      });
  std::future<std::invoke_result_t<F, Args...>> future = task_pkg.get_future();

  queue_lock.lock();
  _tasks.emplace(task_ptr(
      new task_container([task(std::move(task_pkg))]() mutable { task(); })));
  queue_lock.unlock();
  _taskCv.notify_one();

  if (_mapThread.size() < _maxThreads && _freeThreads <= 0) {
    CreateThread(++_currId);
  }

  return future;
}
} // namespace storage
