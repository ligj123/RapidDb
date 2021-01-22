#pragma once

#include <condition_variable>
#include <future> 
#include <mutex>
#include <queue> 
#include <thread>
#include <type_traits>
#include <vector>
#include <string>

namespace utils {
  using namespace std;

  class ThreadPool {
  public:
    ThreadPool(string threadPrefix, uint32_t maxQueueSize = 1000000, size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    // since std::thread objects are not copiable, it doesn't make sense for a
    //  thread_pool to be copiable.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args, std::enable_if_t<std::is_invocable_v<F&&, Args &&...>, int> = 0>
    auto AddTask(F&&, Args &&...);
    void Stop() { _stopThreads = true; }
    uint32_t GetTaskCount() { return (uint32_t)_tasks.size(); }
    void SetMaxQueueSize(uint32_t qsize) { _maxQueueSize = qsize; }
    uint32_t GetMaxQueueSize() { return _maxQueueSize; }
    bool IsFull() { return _maxQueueSize <= _tasks.size(); }
    static string GetThreadName() { return _threadName; }
    static thread_local string _threadName;
  private:
    class _task_container_base {
    public:
      virtual ~_task_container_base() {};

      virtual void operator()() = 0;
    };
    using _task_ptr = std::unique_ptr<_task_container_base>;

    template <typename F, std::enable_if_t<std::is_invocable_v<F&&>, int> = 0>
    class _task_container : public _task_container_base {
    public:
      _task_container(F&& func) : _f(std::forward<F>(func)) {}

      void operator()() override { _f(); }

    private:
      F _f;
    };

    template <typename F> _task_container(F)->_task_container<std::decay<F>>;

    vector<std::thread> _threads;
    queue<_task_ptr> _tasks;
    mutex _task_mutex;
    condition_variable _taskCv;
    bool _stopThreads = false;
    string _threadPrefix;
    uint32_t _maxQueueSize;    
  };

  template <typename F, typename... Args, std::enable_if_t<std::is_invocable_v<F&&, Args &&...>, int>>
  auto ThreadPool::AddTask(F&& function, Args &&...args)
  {
    std::unique_lock<std::mutex> queue_lock(_task_mutex, std::defer_lock);
    std::packaged_task<std::invoke_result_t<F, Args...>()> task_pkg(
      [_f = std::move(function),
      _fargs = std::make_tuple(std::forward<Args>(args)...)]() mutable {
      return std::apply(std::move(_f), std::move(_fargs));
    });
    std::future<std::invoke_result_t<F, Args...>> future = task_pkg.get_future();

    queue_lock.lock();
    _tasks.emplace(_task_ptr(new _task_container([task(std::move(task_pkg))]() mutable { task(); })));
    queue_lock.unlock();
    _taskCv.notify_one();

    return std::move(future);
  }
}