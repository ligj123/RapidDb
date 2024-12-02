#include "ThreadPool.h"
#include "FastQueue.h"
#include "Log.h"
#include "Utilitys.h"

#include <stdexcept>

namespace storage {
std::once_flag mainInstFlag;
atomic_uint32_t ThreadTask::_exclusiveTasksCount{0};
atomic_bool ThreadPool::_stopThreads{false};
ThreadPool *ThreadPool::_instMain{nullptr};

// The default thread id is -1 expected threads from pool.
thread_local string ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;

void ThreadPool::CreateMainPool(const string &threadPrefix, int minThreads,
                                int maxThreads) {
  call_once(mainInstFlag, [threadPrefix, minThreads, maxThreads]() {
    _instMain = new ThreadPool(threadPrefix, minThreads, maxThreads);
  });
}

ThreadPool::ThreadPool(const string &threadPrefix, int minThreads,
                       int maxThreads)
    : _threadPrefix(threadPrefix), _minThreads(minThreads),
      _maxThreads(maxThreads) {
  assert(_minThreads >= 1 && _minThreads <= _maxThreads);
  _stopThreads.store(false, memory_order_relaxed);
  _vctThreadPara.resize(maxThreads);

  for (int i = 0; i < maxThreads; ++i) {
    _vctThreadPara[i]._id = i;
    if (i < minThreads) {
      CreateThread(i);
    }
  }
}

void ThreadPool::CreateThread(int id) {
  std::unique_lock<mutex> thread_lock(_threadMutex);

  if (id == -1) {
    for (size_t i = 0; i < _vctThreadPara.size(); i++) {
      auto &tp = _vctThreadPara[i];
      if (!tp._bRunning) {
        id = i;
        break;
      }
    }

    if (id < 0) {
      return;
    }
  }

  _vctThreadPara[id]._bRunning = true;
  _aliveThreads++;
  _vctThreadPara[id]._thread = new thread([this, id]() {
    _threadName = _threadPrefix + "_" + to_string(id);
    LOG_INFO << "Start thread in thread pool, Name = " << _threadName;
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
            assert(!(*iter)->IsExclusiveTask());
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

      // ManageThreadPool();

      if ((tpara._busyDegree == BusyDegree::BLOCKED && tpara._repeatTime >= 3 ||
           tpara._busyDegree == BusyDegree::BUSY && tpara._repeatTime >= 5) &&
          tpara._vctTask.size() > 1 && !tpara._bExclusiveTask &&
          _poolBusyDegree <= BusyDegree::RELAXED) {
        // This thread is busy and other threads is relax, move one of small
        // tasks to other thread.
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
                 tpara._busyDegree <= BusyDegree::FREE &&
                 _aliveThreads > _minThreads) {
        // The thread pool is free and this thread is free too, free this
        // thread.
        if (IsStoped()) {
          if (tpara._vctTask.size() > 0) {
            continue;
          } else {
            break;
          }
        }
        std::unique_lock<mutex> thread_lock(_threadMutex);
        if (_aliveThreads <= _minThreads) {
          continue;
        }

        MVector<ThreadTask *> vct(tpara._vctTask.begin(), tpara._vctTask.end());
        AddTasks(vct);
        tpara._vctTask.clear();
        break;
      } else if (tpara._bExclusiveTask) {
        // If this thread is running exclusing task, does not need to change
        // anything
        continue;
      }

      std::unique_lock<mutex> queue_lock(_task_mutex);
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
        // If new task is exclusive, move other tasks in queue into other
        // threads.
        MVector<ThreadTask *> vct(tpara._vctTask.begin(), tpara._vctTask.end());
        AddTasks(vct);
        tpara._vctTask.clear();
        tpara._bExclusiveTask = true;
      }

      tpara._vctTask.push_back(task);
    }

    if (!_stopThreads.load(memory_order_relaxed)) {
      tpara._thread->detach();
      delete tpara._thread;
      tpara._thread = nullptr;
    }

    std::unique_lock<mutex> thread_lock(_threadMutex);
    tpara._bRunning = false;
    _aliveThreads--;

    LOG_INFO << "Stop thread in thread pool, Name = " << _threadName;
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
  assert(!_stopThreads.load(memory_order_relaxed));

  std::unique_lock<mutex> queue_lock(_task_mutex);
  _queueTask.push_back(task);
}

void ThreadPool::AddTasks(MVector<ThreadTask *> &vct) {
  assert(!_stopThreads.load(memory_order_relaxed));
  if (vct.size() == 0)
    return;

  std::unique_lock<mutex> queue_lock(_task_mutex);
  for (auto task : vct) {
    _queueTask.push_back(task);
  }
}

void ThreadPool::CheckBusyStatus() {
  DT_MicroSec ts = MicroSecTime();
  if (ts - _checkBusyTime.load(memory_order_acquire) < 10000)
    return;

  int alive = 0;
  int exclusive = 0;
  int busy = 0;
  int free = 0;

  for (size_t i = 0; i < _vctThreadPara.size(); i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (!tpara._bRunning)
      continue;

    alive++;
    if (tpara._bExclusiveTask) {
      exclusive++;
      continue;
    }

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
  } else if (busy > 0) {
    if (free > 0) {
      _poolBusyDegree == BusyDegree::RELAXED;
    } else {
      _poolBusyDegree = BusyDegree::BUSY;
    }
  } else if (exclusive == alive) {
    _poolBusyDegree = BusyDegree::BUSY;
    assert(alive < _maxThreads);
  } else {
    _poolBusyDegree = BusyDegree::RELAXED;
  }

  _checkBusyTime.store(MicroSecTime(), memory_order_release);

  if (_poolBusyDegree == BusyDegree::BUSY && alive < _maxThreads) {
    int num = (int)_queueTask.size() / 3;
    if (num > _maxThreads - alive) {
      num = _maxThreads - alive;
    } else if (num == 0) {
      num = 1;
    }

    for (int i = 0; i < num; i++) {
      CreateThread();
    }
  }
}

void ThreadPool::ManageProc() {
  _threadName = _threadPrefix + "_Mgr";
  LOG_INFO << "Start managing thread for thread pool, Name = " << _threadName;
  assert(_threadName.size() <= 15);
  _threadID = id;
#ifdef LINUX_OS
  pthread_setname_np(pthread_self(), _threadName.c_str());
#endif

  while (true) {
    _nowMicroSec = chrono::duration_cast<chrono::microseconds>(
                       chrono::system_clock::now().time_since_epoch())
                       .count();

    MDeque<ThreadTask *> queue;
    if (_queueTask.size() > 0) {
      unique_lock<SpinMutex> lock(_taskMutex);
      queue.swap(_queueTask);
    }

    _rapidTaskQueue.Pop(queue);

    if (_nowMicroSec - _checkBusyTime) {
      CheckBusyStatus();
      _checkBusyTime = _nowMicroSec;

      if (_poolBusyDegree >= BusyDegree::BUSY && _aliveThreads < _maxThreads &&
          queue.size() > 0) {
        int num = (int)_queue.size() / 3;
        if (num > _maxThreads - _aliveThreads) {
          num = _maxThreads - _aliveThreads;
        } else if (num == 0) {
          num = 1;
        }

        for (int i = 0; i < num; i++) {
          CreateWorkThread();
        }
      }
    }

    int32_t idx = 0;
    int32_t ring = 0;
    while (queue.size() > 0) {
      ThreadTask *task = queue.front();
      queue.pop_front();

      if (task->IsExclusiveTask()) {
        int32_t pos = -1;
        BusyDegree degree = BusyDegree::BLOCKED;
        for (int32_t i = 0; i < _maxThreads; i++) {
          if (_vctThreadPara[i]._bRunning &&
              !_vctThreadPara[i]._bExclusiveTask) {
            if (degree > _vctThreadPara[i]._busyDegree) {
              pos = i;
              degree = _vctThreadPara[i]._busyDegree;
            }
          }
        }

        assert(pos >= 0);
        _vctThreadPara[i]._bExclusiveTask = true;
        _vctThreadPara[i]._lineQueueTask.Push(task);
      } else {
        while (true) {
          if (idx == _maxThreads) {
            idx = 0;
            ring++;
          }

          if (!_vctThreadPara[idx]._bRunning ||
              _vctThreadPara[idx]._bExclusiveTask) {
            idx++;
          }

          if (_vctThreadPara[idx]._busyDegree < BusyDegree::RELAXED ||
              (_vctThreadPara[idx]._busyDegree < BusyDegree::RELAXED &&
               ring > 0) ||
              (ring > 1)) {
            _vctThreadPara[i]._lineQueueTask.Push(task);
            break;
          }
        }
      }
    }
  }
}
} // namespace storage
