#include "ThreadPool.h"

namespace utils {
thread_local string ThreadPool::_threadName = "main";

ThreadPool::ThreadPool(string threadPrefix, uint32_t maxQueueSize,
                       size_t threadCount)
    : _threadPrefix(threadPrefix), _maxQueueSize(maxQueueSize),
      _stopThreads(false) {
  for (size_t i = 0; i < threadCount; ++i) {
    _threads.emplace_back(std::thread([this, i]() {
      std::unique_lock<std::mutex> queue_lock(_task_mutex, std::defer_lock);
      _threadName = _threadPrefix + "_" + to_string(i);

      while (true) {
        queue_lock.lock();
        _taskCv.wait(queue_lock,
                     [&]() -> bool { return !_tasks.empty() || _stopThreads; });

        if (_stopThreads && _tasks.empty())
          return;

        auto temp_task = std::move(_tasks.front());

        _tasks.pop();
        queue_lock.unlock();

        (*temp_task)();
      }
    }));
  }
}

ThreadPool::~ThreadPool() {
  _stopThreads = true;
  _taskCv.notify_all();

  for (std::thread &thread : _threads) {
    thread.join();
  }
}
} // namespace utils