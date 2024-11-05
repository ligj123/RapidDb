#include "ThreadPool.h"
#include "FastQueue.h"
#include "Log.h"
#include "Utilitys.h"

#include <stdexcept>

namespace storage {
atomic_uint32_t ThreadTask::_exclusiveTasksCount{0};
atomic_bool ThreadPool::_stopThreads{false};

// The default thread id is -1 expected threads from pool.
thread_local string ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;

ThreadPool::ThreadPool(string threadPrefix, int minThreads, int maxThreads)
    : _threadPrefix(threadPrefix), _minThreads(minThreads),
      _maxThreads(maxThreads) {
  assert(_minThreads >= 1 && _minThreads <= _maxThreads);
  _vctThreadPara.resize(maxThreads);

  for (int i = 0; i < minThreads; ++i) {
    _vctThreadPara[i]._id = i;
    CreateThread(i);
  }
}

void ThreadPool::CreateThread(int id) {
  std::unique_lock<SpinMutex> thread_lock(_threadMutex, try_to_lock);
  if (!thread_lock.owns_lock()) {
    return;
  }

  if (id == -1) {
    for (size_t i = 0; i < _vctThreadPara.size(); i++) {
      auto &tp = _vctThreadPara[i];
      if (!tp._bRunning) {
        id = i;
        break;
      }
    }
  }

  _vctThreadPara[id]._bRunning = true;
  _vctThreadPara[id]._thread = new thread([this, id]() {
    _threadName = _threadPrefix + "_" + to_string(id);
    assert(_threadName.size() <= 15);
    _threadID = id;
#ifdef LINUX_OS
    pthread_setname_np(pthread_self(), _threadName.c_str());
#endif
    ThreadPara &tpara = _vctThreadPara[id];
    assert(tpara._id == id);

    while (true) {
      if (tpara._vctTask.size() == 0) {
        if (tpara._busyDegree == BusyDegree::EMPTY) {
          tpara._repeatTime++;
        } else {
          tpara._busyDegree = BusyDegree::EMPTY;
          tpara._repeatTime = 1;
        }
      } else {
        DT_MicroSec dtStart = MicroSecTime();
        if (tpara._bExclusiveTask) {
          assert(tpara._vctTask.size() == 1);
          TaskStatus ts = tpara._vctTask[0]->Run();
          if (ts == TaskStatus::FINISHED) {
            if (tpara._vctTask[0]->IsNeedDelete()) {
              delete tpara._vctTask[0];
            }

            tpara._vctTask.clear();
            tpara._bExclusiveTask = false;
          }
        } else {
          for (auto iter = tpara._vctTask.begin();
               iter != tpara._vctTask.end();) {
            TaskStatus ts = (*iter)->Run();
            if (ts == TaskStatus::FINISHED) {
              if ((*iter)->IsNeedDelete()) {
                delete (*iter);
              }

              iter = tpara._vctTask.erase(iter);
            } else {
              iter++;
            }
          }
        }

        BusyDegree bd = GetBusyDegree(MicroSecTime() - dtStart);
        if (bd == tpara._busyDegree) {
          tpara._repeatTime++;
        } else {
          if (bd >= BusyDegree::BUSY) {
            tpara._repeatTime++;
          } else {
            tpara._repeatTime = 1;
          }

          tpara._busyDegree = bd;
        }
      }

      ManageThreadPool();

      if ((tpara._busyDegree == BusyDegree::BLOCKED && tpara._repeatTime >= 3 ||
           tpara._busyDegree == BusyDegree::BUSY && tpara._repeatTime >= 5) &&
          tpara._vctTask.size() > 1 && !tpara._bExclusiveTask &&
          _poolBusyDegree <= BusyDegree::RELAXED) {
        // This thread is busy and thread pool is relax, move one of task to
        // other thread.
        auto iter = tpara._vctTask.begin();
        auto itSel = iter;
        iter++;

        for (; iter != tpara._vctTask.end(); iter++) {
          if ((*iter)->GetBusyDegree() < (*itSel)->GetBusyDegree()) {
            itSel = iter;
          }
        }

        AddTask(*itSel);
        tpara._vctTask.erase(itSel);
        continue;
      } else if (_poolBusyDegree == BusyDegree::FREE &&
                 !tpara._bExclusiveTask &&
                 tpara._busyDegree <= BusyDegree::FREE) {
        // The thread pool is free and this thread is free too, free this
        // thread.
        MVector<ThreadTask *> vct(tpara._vctTask.begin(), tpara._vctTask.end());
        AddTasks(vct);
        tpara._vctTask.clear();
        tpara._bRunning = false;
        break;
      } else if (tpara._bExclusiveTask) {
        // If this thread is running exclusing task, does not need to change
        // anything
        continue;
      }

      std::unique_lock<SpinMutex> queue_lock(_task_mutex);
      if (_queueTask.size() == 0) {
        if (_stopThreads.load(memory_order_relaxed)) {
          break;
        }

        if (tpara._busyDegree == BusyDegree::EMPTY) {
          _taskCv.wait_for(queue_lock, 10ms, [this]() -> bool {
            return _queueTask.size() > 0 ||
                   _stopThreads.load(memory_order_relaxed);
          });

          if (_queueTask.size() == 0)
            continue;
        } else {
          continue;
        }
      }

      ThreadTask *task = _queueTask.front();
      _queueTask.pop_front();

      if (task->IsExclusiveTask()) {
        MVector<ThreadTask *> vct(tpara._vctTask.begin(), tpara._vctTask.end());
        AddTasks(vct);
        tpara._vctTask.clear();
        tpara._bExclusiveTask = true;
      }

      tpara._vctTask.push_back(task);
    }

    tpara._bRunning = false;
    if (!_stopThreads.load(memory_order_relaxed)) {
      tpara._thread->detach();
      delete tpara._thread;
      tpara._thread = nullptr;
    }
  });
}

ThreadPool::~ThreadPool() {
  for (int i = 0; i < _maxThreads; i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (!tpara._bRunning)
      continue;

    tpara._thread->join();
    delete tpara._thread;
  }

  assert(_queueTask.size() == 0);
}

void ThreadPool::AddTask(ThreadTask *task) {
  assert(!_stopThreads);

  std::unique_lock<SpinMutex> queue_lock(_task_mutex);
  if (_queueTask.size() == 0) {
    _taskTime = MicroSecTime();
  }
  _queueTask.push_back(task);
}

void ThreadPool::AddTasks(MVector<ThreadTask *> &vct) {
  if (vct.size() == 0)
    return;
  assert(!_stopThreads);
  {
    std::unique_lock<SpinMutex> queue_lock(_task_mutex);
    if (_queueTask.size() == 0) {
      _taskTime = MicroSecTime();
    }

    for (auto task : vct) {
      _queueTask.push_back(task);
    }
  }
  _taskCv.notify_all();
}

void ThreadPool::ManageThreadPool() {
  if (MicroSecTime() - _checkBusyTime.load(memory_order_acquire) < 100000)
    return;

  int alive = 0;
  int busy = 0;
  int free = 0;

  for (size_t i = 0; i < _vctThreadPara.size(); i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (!tpara._bRunning)
      continue;

    alive++;
    if (tpara._busyDegree == BusyDegree::BUSY && tpara._repeatTime > 1 ||
        tpara._busyDegree == BusyDegree::BLOCKED) {
      busy++;
    } else if (tpara._busyDegree <= BusyDegree::FREE && tpara._repeatTime > 3 &&
               !tpara._bExclusiveTask) {
      free++;
    }
  }

  if (free >= 2) {
    if (alive == _minThreads) {
      _poolBusyDegree = BusyDegree::RELAXED;
    } else {
      _poolBusyDegree = BusyDegree::FREE;
    }
  } else if (busy > alive / 2) {
    if (free > 0) {
      _poolBusyDegree == BusyDegree::RELAXED;
    } else {
      _poolBusyDegree = BusyDegree::BUSY;
    }
  } else {
    _poolBusyDegree = BusyDegree::RELAXED;
  }

  _checkBusyTime.store(MicroSecTime(), memory_order_release);

  if (_poolBusyDegree == BusyDegree::BUSY && alive < _maxThreads) {
    CreateThread();
  }
}
} // namespace storage
