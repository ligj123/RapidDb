#include "ThreadPool.h"
#include "FastQueue.h"
#include "Log.h"
#include <stdexcept>

namespace storage {
// The default thread id is -1 expected threads from pool.
thread_local string ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;
thread_local Task *ThreadPool::_currTask = nullptr;
ThreadPool *ThreadPool::_instMain = nullptr;
SpinMutex ThreadPool::_smMain;
#ifdef CACHE_TRACE
set<int> ThreadPool::_setId{};
#endif

ThreadPool::ThreadPool(string threadPrefix, uint32_t maxQueueSize,
                       int minThreads, int maxThreads, int startId)
    : _threadPrefix(threadPrefix), _maxQueueSize(maxQueueSize),
      _stopThreads(false), _minThreads(minThreads), _maxThreads(maxThreads),
      _aliveThreads(0), _freeThreads(0), _tasksNum(0), _startId(startId) {
  if (_minThreads < 1 || _minThreads > _maxThreads || _maxThreads < 1) {
    throw invalid_argument(
        "Please set min threads and max thread in right range!");
  }

  _fastQueue = new FastQueue<Task, 1000>(this);
  _vctThread.resize(_maxThreads, nullptr);
  for (int i = 0; i < minThreads; ++i) {
    CreateThread(i);
  }
}

void ThreadPool::CreateThread() {
  std::unique_lock<SpinMutex> thread_lock(_threadMutex, try_to_lock);
  if (!thread_lock.owns_lock()) {
    return;
  }

  int id = _aliveThreads;

  thread *t = new thread([this, id]() {
    AddThread(_threadPrefix + "_" + to_string(id), id + _startId);
    vector<Task *> vct;
    vct.reserve(10);

    while (true) {
      vct.clear();
      std::unique_lock<SpinMutex> queue_lock(_task_mutex);
      _freeThreads++;
      _taskCv.wait_for(queue_lock, 500ms, [this]() -> bool {
        return _tasksNum > 0 || !_fastQueue->RoughEmpty() || _stopThreads;
      });

      if (_tasksNum == 0 && _fastQueue->RoughEmpty()) {
        _freeThreads--;
        queue_lock.unlock();
        std::unique_lock<SpinMutex> thread_lock(_threadMutex);
        if ((_aliveThreads > _minThreads && _aliveThreads == id + 1) ||
            _stopThreads) {
          break;
        } else {
          continue;
        }
      }

      if (_urgentTasks.size() > 0) {
        vct.push_back(_urgentTasks.front());
        _urgentTasks.pop_front();
      } else {
        if (_smallTasks.size() == 0 && _largeTasks.size() == 0) {
          queue<Task *> q;
          _fastQueue->Pop(q);
          _tasksNum += (int32_t)q.size();

          while (q.size() > 0) {
            Task *task = q.front();
            q.pop();

            if (task->IsSmallTask()) {
              _smallTasks.push_back(task);
            } else {
              _largeTasks.push_back(task);
            }
          }
        }

        if (_threadID % 2 && _largeTasks.size() > 0 ||
            _smallTasks.size() == 0) {
          vct.push_back(_largeTasks.front());
          _largeTasks.pop_front();
        } else {
          while (vct.size() < 5 && _smallTasks.size() > 0) {
            vct.push_back(_smallTasks.front());
            _smallTasks.pop_front();
          }
        }
      }

      _freeThreads--;
      _tasksNum -= (int)vct.size();
      queue_lock.unlock();

      for (Task *task : vct) {
        _currTask = task;
        task->SetStatus(TaskStatus::RUNNING);
        task->Run();
        assert(task->Status() > TaskStatus::RUNNING);
        _currTask = nullptr;

        if (task->Status() == TaskStatus::PAUSE_WITH_ADD ||
            task->Status() == TaskStatus::INTERVAL) {
          AddTask(task);
        } else if (task->Status() == TaskStatus::FINISHED) {
          AddTasks(task->GetWaitTasks());
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

    for (function<void(uint16_t threads)> func : _vctLambda) {
      func(_aliveThreads);
    }

    RemoveThread(id);
  });

  _vctThread[id] = t;
  _aliveThreads++;
  for (function<void(uint16_t threads)> func : _vctLambda) {
    func(_aliveThreads);
  }

  thread_lock.unlock();
}

ThreadPool::~ThreadPool() {
  for (int i = 0; i < _maxThreads; i++) {
    thread *t = _vctThread[i];
    if (t == nullptr)
      continue;

    t->join();
    delete t;
    _aliveThreads--;
  }

  _vctLambda.clear();
  assert(GetTaskCount() == 0);
  delete _fastQueue;
}

void ThreadPool::AddTask(Task *task, bool urgent) {
  assert(!_stopThreads);

  if (urgent) {
    std::unique_lock<SpinMutex> queue_lock(_task_mutex);
    _urgentTasks.push_back(task);
  } else {
    _fastQueue->Push(task);
  }
  _taskCv.notify_one();

  if (_aliveThreads < _maxThreads && _freeThreads <= 0) {
    CreateThread();
  }
}

void ThreadPool::AddTasks(MVector<Task *> &vct) {
  if (vct.size() == 0)
    return;
  assert(!_stopThreads);

  for (auto task : vct) {
    _fastQueue->Push(task);
  }
  _taskCv.notify_all();

  if (_aliveThreads < _maxThreads && _freeThreads <= 0) {
    CreateThread();
  }
}

bool ThreadPool::IsFull() {
  return _maxQueueSize <= _tasksNum + _fastQueue->RoughSize();
}
} // namespace storage
