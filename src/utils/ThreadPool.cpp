#include "ThreadPool.h"
#include "FastQueue.h"
#include "Log.h"
#include "Utilitys.h"

#include <stdexcept>

namespace storage {
atomic_uint32_t ThreadTask::_exclusiveTasksCount{0};
atomic_bool ThreadPool::_stopThreads{false};
ThreadPool *ThreadPool::_instMain{nullptr};
DT_MicroSec ThreadPool::_nowMicroSec{0};

// The default thread id is -1 expected threads from pool.
thread_local MString ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;

ThreadPool *ThreadPool::CreateMainPool(const MString &threadPrefix,
                                       int minThreads, int maxThreads) {
  assert(_instMain == nullptr);
  _instMain = new ThreadPool(threadPrefix, minThreads, maxThreads);
  return _instMain;
}

void ThreadPool::CloseMainPool(bool ignoreTasks) {
  assert(_instMain != nullptr);
  if (ignoreTasks) {
    _instMain->_rapidTaskQueue.Pop(_instMain->_queueTask);
    _instMain->_queueTask.clear();
  }
  _instMain->SetStop();
  delete _instMain;
  _instMain = nullptr;
  _nowMicroSec = 0;
}

ThreadPool::ThreadPool(const MString &threadPrefix, int minThreads,
                       int maxThreads)
    : _threadPrefix(threadPrefix), _minThreads(minThreads),
      _maxThreads(maxThreads), _rapidTaskQueue(maxThreads, maxThreads) {
  assert(_minThreads >= 1 && _minThreads <= _maxThreads);
  _stopThreads.store(false, memory_order_relaxed);
  _vctThreadPara.resize(maxThreads);

  for (int i = 0; i < maxThreads; ++i) {
    _vctThreadPara[i]._id = i;
    if (i < minThreads) {
      CreateWorkThread(i);
    }
  }

  _threadMgr = new thread([this]() { ManageProc(); });
}

void ThreadPool::CreateWorkThread(int id) {
  if (id == -1) {
    for (size_t i = 0; i < _vctThreadPara.size(); i++) {
      auto &tp = _vctThreadPara[i];
      if (!tp._bRunning.load(memory_order_relaxed)) {
        id = i;
        break;
      }
    }

    if (id < 0) {
      return;
    }
  }

  _vctThreadPara[id]._bStop = false;
  _vctThreadPara[id]._bRunning.store(true, memory_order_relaxed);

  _aliveThreads++;
  _vctThreadPara[id]._thread = new thread([this, id]() { WorkProc(id); });
}

ThreadPool::~ThreadPool() {
  assert(_stopThreads.load(memory_order_relaxed));

  _threadMgr->join();
  delete _threadMgr;

  for (int i = 0; i < _maxThreads; i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (!tpara._bRunning.load(memory_order_relaxed)) {
      continue;
    }

    tpara._thread->join();
    delete tpara._thread;
  }

  assert(_queueTask.size() == 0 && _rapidTaskQueue.IsEmpty());
}

void ThreadPool::CheckBusyStatus() {
  if (_nowMicroSec - _checkBusyTime < 10000)
    return;

  int alive = 0;
  int exclusive = 0;
  int busy = 0;
  int free = 0;

  for (size_t i = 0; i < _vctThreadPara.size(); i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (tpara._bStop)
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
    assert(alive <= _maxThreads);
  } else {
    _poolBusyDegree = BusyDegree::RELAXED;
  }

  _checkBusyTime = _nowMicroSec;
}

void ThreadPool::ManageProc() {
  _threadName = _threadPrefix + "_Mgr";
  LOG_INFO << "Start managing thread for thread pool, Name = " << _threadName;
  assert(_threadName.size() <= 15);
  _threadID = -1;
#ifdef LINUX_OS
  pthread_setname_np(pthread_self(), _threadName.c_str());
#endif

  int32_t stopTryTime = 10;
  while (true) {
    if (IsStoped()) {
      stopTryTime--;
      if (stopTryTime <= 0) {
        for (int32_t i = 0; i < _maxThreads; i++) {
          _vctThreadPara[i]._bStop = true;
        }

        _aliveThreads = 0;
        break;
      }
    }

    _nowMicroSec = chrono::duration_cast<chrono::microseconds>(
                       chrono::system_clock::now().time_since_epoch())
                       .count();

    MList<ThreadTask *> queue;
    _rapidTaskQueue.Pop(queue);

    if (_queueTask.size() > 0) {
      unique_lock<SpinMutex> lock(_taskMutex);
      queue.swap(_queueTask);
    }

    if (_nowMicroSec - _checkBusyTime >= 10000) {
      CheckBusyStatus();
      _checkBusyTime = _nowMicroSec;

      if (_poolBusyDegree >= BusyDegree::BUSY && _aliveThreads < _maxThreads &&
          queue.size() > 0) {
        int num = (int)queue.size() / 3;
        if (num > _maxThreads - _aliveThreads) {
          num = _maxThreads - _aliveThreads;
        } else if (num == 0) {
          num = 1;
        }

        for (int i = 0; i < num; i++) {
          CreateWorkThread();
        }
      } else if (_poolBusyDegree <= BusyDegree::FREE &&
                 _aliveThreads > _minThreads) {
        BusyDegree degree = BusyDegree::RELAXED;
        int pos = -1;
        for (int32_t i = 0; i < _maxThreads; i++) {
          if (!_vctThreadPara[i]._bStop &&
              _vctThreadPara[i]._busyDegree <= degree) {
            degree = _vctThreadPara[i]._busyDegree;
            pos = i;
          }
        }

        if (pos >= 0) {
          _vctThreadPara[pos]._bStop = true;
          _aliveThreads--;
        }
      }
    }

    int32_t idx = 0;
    if ((queue.size() >
             (_aliveThreads - ThreadTask::GetExclusiveTaskCount()) * 3 &&
         _aliveThreads < _maxThreads)) {
      int num = (int)queue.size() / 3;
      if (num > _maxThreads - _aliveThreads) {
        num = _maxThreads - _aliveThreads;
      } else if (num == 0) {
        num = 1;
      }

      for (int i = 0; i < num; i++) {
        CreateWorkThread();
      }
    }

    while (queue.size() > 0) {
      ThreadTask *task = queue.front();
      queue.pop_front();

      if (task->IsExclusiveTask()) {
        if (_aliveThreads < ThreadTask::GetExclusiveTaskCount()) {
          assert(ThreadTask::GetExclusiveTaskCount() < _maxThreads);
          int num = ThreadTask::GetExclusiveTaskCount() - _aliveThreads + 1;
          for (int i = 0; i < num; i++) {
            CreateWorkThread();
          }
        }

        int32_t pos = -1;
        BusyDegree degree = BusyDegree::BLOCKED;
        for (int32_t i = 0; i < _maxThreads; i++) {
          if (!_vctThreadPara[i]._bStop && !_vctThreadPara[i]._bExclusiveTask) {
            if (degree > _vctThreadPara[i]._busyDegree) {
              pos = i;
              degree = _vctThreadPara[i]._busyDegree;
            }
          }
        }

        assert(pos >= 0);
        _vctThreadPara[pos]._bExclusiveTask = true;
        _vctThreadPara[pos]._lineQueueTask.Push(task);
        _vctThreadPara[pos].ClearMask();
      } else {
        int32_t ring = 0;

        while (true) {
          if (idx == _maxThreads) {
            idx = 0;
            ring++;
          }

          if (_vctThreadPara[idx]._bStop ||
              _vctThreadPara[idx]._bExclusiveTask) {
            idx++;
            continue;
          }

          if (ring == 0 &&
              _vctThreadPara[idx].IsMaskConflict(task->GetTaskMask())) {
            idx++;
            continue;
          }

          if (_vctThreadPara[idx]._busyDegree < BusyDegree::RELAXED ||
              (_vctThreadPara[idx]._busyDegree < BusyDegree::RELAXED &&
               ring > 0) ||
              (ring > 1)) {
            _vctThreadPara[idx]._lineQueueTask.Push(task);
            _vctThreadPara[idx].SetMask(task->GetTaskMask());
            idx++;
            break;
          }

          idx++;
        }
      }
    }

    this_thread::yield();
  }
}

void ThreadPool::WorkProc(uint16_t tid) {
  _threadName = _threadPrefix + "_" + ToMString(tid);
  LOG_INFO << "Start thread in thread pool, Name = " << _threadName;
  assert(_threadName.size() <= 15);
  _threadID = tid;
#ifdef LINUX_OS
  pthread_setname_np(pthread_self(), _threadName.c_str());
#endif
  ThreadPara &tpara = _vctThreadPara[tid];
  assert(tpara._id == tid);

  while (true) {
    MList<ThreadTask *> q;
    tpara._lineQueueTask.Pop(q);
    if (q.size() > 0) {
      tpara._vctTask.insert(tpara._vctTask.end(), q.begin(), q.end());
    }

    if (tpara._vctTask.size() == 0) {
      if (tpara._busyDegree == BusyDegree::EMPTY) {
        tpara._repeatTime++;
      } else {
        tpara._busyDegree = BusyDegree::EMPTY;
        tpara._repeatTime = 1;
      }
    } else {
      DT_MicroSec dtStart = _nowMicroSec;
      if (tpara._bExclusiveTask) {
        if (tpara._vctTask.size() > 1) {
          ThreadTask *task = nullptr;
          for (auto iter = tpara._vctTask.begin(); iter != tpara._vctTask.end();
               iter++) {
            if ((*iter)->IsExclusiveTask()) {
              assert(task == nullptr);
              task = *iter;
            } else {
              AddTask(GetThreadId(), *iter);
            }
          }

          tpara._vctTask.clear();
          assert(task != nullptr && task->IsExclusiveTask());
          tpara._vctTask.push_back(task);
        }

        TaskStatus ts = tpara._vctTask[0]->Run();
        if (ts == TaskStatus::FINISHED) {
          if (tpara._vctTask[0]->IsNeedDelete()) {
            delete tpara._vctTask[0];
          }

          tpara._vctTask.clear();
          tpara._bExclusiveTask = false;
          tpara._busyDegree = BusyDegree::EMPTY;
          tpara._repeatTime = 1;
        } else {
          BusyDegree bd = CalcBusyDegree(MicroSecTime() - dtStart);
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
          continue;
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

        BusyDegree bd = CalcBusyDegree(MicroSecTime() - dtStart);
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
    }

    if ((tpara._busyDegree == BusyDegree::BLOCKED && tpara._repeatTime >= 3 ||
         tpara._busyDegree == BusyDegree::BUSY && tpara._repeatTime >= 5) &&
        tpara._vctTask.size() > 1 && !tpara._bExclusiveTask &&
        _poolBusyDegree <= BusyDegree::RELAXED &&
        tpara._dtRemoveTask < _checkBusyTime) {
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

      AddTask(GetThreadId(), *itSel);
      tpara._vctTask.erase(itSel);
      tpara._dtRemoveTask = _checkBusyTime;
    } else if (tpara._bStop) {
      if (tpara._vctTask.size()) {
        continue;
      } else {
        break;
      }
    }
  }

  if (!_stopThreads.load(memory_order_relaxed)) {
    tpara._thread->detach();
    delete tpara._thread;
    tpara._thread = nullptr;
    tpara._bRunning.store(false, memory_order_release);
  }

  LOG_INFO << "Stop thread in thread pool, Name = " << _threadName;
}
} // namespace storage
