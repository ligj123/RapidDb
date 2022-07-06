#include "ThreadPool.h"
#include <stdexcept>

namespace storage {
thread_local string ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;
thread_local Task *ThreadPool::_currTask = nullptr;
ThreadPool *ThreadPool::_instMain = nullptr;
SpinMutex ThreadPool::_smMain;

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
      if (_vctThread[i] == nullptr) {
        id = i;
        break;
      }
    }
  }

  thread *t = new thread([this, id]() {
    _threadName = _threadPrefix + "_" + to_string(id);
    _threadID = id;
    MVector<Task *>::Type vct;

    while (true) {
      std::unique_lock<SpinMutex> queue_lock(_task_mutex);
      _freeThreads++;
      _taskCv.wait_for(queue_lock, 500ms, [&]() -> bool {
        return GetTaskCount() > 0 || _stopThreads;
      });

      if (GetTaskCount() == 0) {
        _freeThreads--;
        queue_lock.unlock();
        std::unique_lock<SpinMutex> thread_lock(_threadMutex);
        if (_aliveThreads > _minThreads || _stopThreads) {
          break;
        } else {
          continue;
        }
      }

      vct.clear();
      if (_urgentTasks.size() > 0) {
        vct.push_back(_urgentTasks.front());
        _urgentTasks.pop_front();
      } else if (_threadID % 2 && _largeTasks.size() > 0 ||
                 _smallTasks.size() == 0) {
        vct.push_back(_largeTasks.front());
        _largeTasks.pop_front();
      } else {
        while (vct.size() < 5 && _smallTasks.size() > 0) {
          vct.push_back(_smallTasks.front());
          _smallTasks.pop_front();
        }
      }

      _freeThreads--;
      queue_lock.unlock();

      for (Task *task : vct) {
        _currTask = task;
        task->Run();
        assert(task->Status() > TaskStatus::RUNNING);
        _currTask = nullptr;

        if (task->Status() == TaskStatus::PAUSE_WITH_ADD ||
            task->Status() == TaskStatus::INTERVAL) {
          AddTask(task);
        } else if (task->Status() == TaskStatus::STOPED) {
          delete task;
        }
      }
      vct.clear();
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

  assert(GetTaskCount() == 0);
}

void ThreadPool::AddTask(Task *task, bool urgent) {
  assert(!_stopThreads);
  {
    std::unique_lock<SpinMutex> queue_lock(_task_mutex);
    if (urgent) {
      _urgentTasks.push_back(task);
    } else if (task->IsSmallTask()) {
      _smallTasks.push_back(task);
    } else {
      _largeTasks.push_back(task);
    }
  }

  if (_aliveThreads < _maxThreads && _freeThreads <= 0) {
    CreateThread();
  }
  _taskCv.notify_one();
}

void ThreadPool::AddTasks(MVector<Task *>::Type &vct) {
  assert(!_stopThreads);
  if (vct.size() == 0)
    return;

  {
    std::unique_lock<SpinMutex> queue_lock(_task_mutex);
    for (auto task : vct) {
      if (task->IsSmallTask()) {
        _smallTasks.push_back(task);
      } else {
        _largeTasks.push_back(task);
      }
    }
  }

  if (_aliveThreads < _maxThreads && _freeThreads <= 0) {
    CreateThread();
  }
  _taskCv.notify_all();
}
} // namespace storage
