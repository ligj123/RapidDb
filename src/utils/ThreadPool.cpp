#include "ThreadPool.h"
#include <stdexcept>

namespace storage {
thread_local string ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;

ThreadPool::ThreadPool(string threadPrefix, uint32_t maxQueueSize,
                       int minThreads, int maxThreads)
    : _threadPrefix(threadPrefix), _maxQueueSize(maxQueueSize),
      _stopThreads(false), _minThreads(minThreads), _maxThreads(maxThreads),
      _aliveThreads(0), _freeThreads(0) {
  if (_minThreads < 1 || _minThreads > _maxThreads || _maxThreads < 1) {
    throw invalid_argument(
        "Please set min threads and max thread in right range!");
  }

  _vctThread.resize(_maxThreads, nullptr);
  for (int i = 0; i < maxThreads; ++i) {
    if (i < minThreads) {
      CreateThread(i);
    }
  }
}

void ThreadPool::CreateThread(int id) {
  if (id < 0) {
    for (int i = 0; i < _maxThreads; i++) {
      if (_vctThread[i] == nullptr)
        id = i;
    }
  }

  thread *t = new thread([this, id]() {
    std::unique_lock<SpinMutex> queue_lock(_task_mutex, std::defer_lock);
    _threadName = _threadPrefix + "_" + to_string(id);
    _threadID = id;
    MVector<Task *>::Type vct;

    while (true) {
      queue_lock.lock();
      _freeThreads++;
      _taskCv.wait_for(queue_lock, 500ms, [&]() -> bool {
        return !_tasks.empty() || _stopThreads;
      });

      if (_tasks.empty()) {
        _freeThreads--;
        queue_lock.unlock();
        std::unique_lock<SpinMutex> thread_lock(_threadMutex);
        if (_aliveThreads > _minThreads || _stopThreads) {
          break;
        } else {
          continue;
        }
      }

      auto task = _tasks.front();
      if (task->IsSmallTask()) {
        while (true) {
          vct.push_back(task);
          _tasks.pop_front();
          if (_tasks.size() == 0 || _tasks.front()->IsSmallTask())
            break;
          task = _tasks.front();
        }
      } else {
        _tasks.pop_front();
      }
      _freeThreads--;
      queue_lock.unlock();

      if (vct.size() > 0) {
        for (auto task : vct) {
          task->Run();
          task->DecRefCount();
        }
        vct.clear();
      } else {
        task->Run();
      }
    }

    std::unique_lock<SpinMutex> thread_lock(_threadMutex);

    if (!_stopThreads) {
      thread *t = _vctThread[id];
      _vctThread[id] = nullptr;
      _aliveThreads--;
      t->detach();
      delete t;
    }
  });

  std::unique_lock<SpinMutex> thread_lock(_threadMutex);
  _vctThread[id] = t;
  _aliveThreads++;
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<SpinMutex> thread_lock(_threadMutex);
    _stopThreads = true;
  }
  _taskCv.notify_all();

  for (int i = 0; i < _maxThreads; i++) {
    thread *t = _vctThread[i];
    if (t == nullptr)
      continue;

    t->join();
    delete t;
    _aliveThreads--;
  }
}

void ThreadPool::AddTask(Task *task) {
  std::unique_lock<SpinMutex> queue_lock(_task_mutex, std::defer_lock);
  queue_lock.lock();
  _tasks.push_back(task);
  queue_lock.unlock();
  _taskCv.notify_one();

  if (_aliveThreads < _maxThreads && _freeThreads <= 0) {
    CreateThread();
  }
}

void ThreadPool::AddTasks(MVector<Task *>::Type &vct) {
  std::unique_lock<SpinMutex> queue_lock(_task_mutex, std::defer_lock);
  queue_lock.lock();
  for (auto task : vct) {
    _tasks.push_back(task);
  }

  queue_lock.unlock();
  _taskCv.notify_one();

  if (_aliveThreads < _maxThreads && _freeThreads <= 0) {
    CreateThread();
  }
}
} // namespace storage
