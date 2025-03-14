#include "ThreadPool.h"
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
  _nowMicroSec = chrono::duration_cast<chrono::microseconds>(
                     chrono::system_clock::now().time_since_epoch())
                     .count();

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

  _aliveThreads++;
  assert(_vctThreadPara[id]._vctTask.size() == 0);
  assert(_vctThreadPara[id]._lineQueueTask.IsEmpty());
  _vctThreadPara[id]._bStop = false;
  _vctThreadPara[id]._bRunning.store(true, memory_order_relaxed);
  _vctThreadPara[id].ClearMask();
  _vctThreadPara[id]._bExclusiveTask = false;
  _vctThreadPara[id]._dtUsed = 0;
  _vctThreadPara[id]._thread = new thread([this, id]() { WorkProc(id); });
}

ThreadPool::~ThreadPool() {
  assert(_stopThreads.load(memory_order_relaxed));
  if (_threadMgr != nullptr) {
    _threadMgr->join();
    delete _threadMgr;
  }
  assert(_queueTask.size() == 0 && _rapidTaskQueue.IsEmpty());
}

void ThreadPool::CheckBusyStatus() {
  if (_nowMicroSec - _checkBusyTime < 10000)
    return;

  int normal = 0;
  int exclusive = 0;
  int busy = 0;
  int free = 0;
  int posMax = -1;
  double dtMax = -1;
  double dtMin = UINT64_MAX;

  for (size_t i = 0; i < _vctThreadPara.size(); i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (tpara._bStop)
      continue;

    if (tpara._bExclusiveTask) {
      exclusive++;
      continue;
    }

    normal++;
    BusyDegree bd = CalcBusyDegree(tpara._dtUsed);

    if (bd >= BusyDegree::BUSY) {
      busy++;
    } else if (bd <= BusyDegree::FREE) {
      free++;
    }

    if (tpara._dtUsed > dtMax) {
      dtMax = tpara._dtUsed;
      posMax = i;
    } else if (tpara._dtUsed < dtMin) {
      dtMin = tpara._dtUsed;
    }
  }

  if (free == normal) {
    _poolBusyDegree = BusyDegree::EMPTY;
  } else if (free > 0) {
    if (free > normal / 2) {
      _poolBusyDegree = BusyDegree::FREE;
    } else {
      _poolBusyDegree = BusyDegree::RELAXED;
    }
  } else if (busy > 0) {
    if (busy == normal) {
      _poolBusyDegree = BusyDegree::BLOCKED;
    } else if (busy < normal / 3) {
      _poolBusyDegree = BusyDegree::RELAXED;
    } else {
      _poolBusyDegree = BusyDegree::BUSY;
    }
  } else {
    _poolBusyDegree = BusyDegree::RELAXED;
  }

  if (dtMin > 0 && dtMax / dtMin > 10) {
    _vctThreadPara[posMax]._bSelRmTask = true;
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

  while (true) {
    _nowMicroSec = chrono::duration_cast<chrono::microseconds>(
                       chrono::system_clock::now().time_since_epoch())
                       .count();

    if (IsStoped()) {
      for (int32_t i = 0; i < _maxThreads; i++) {
        _vctThreadPara[i]._bStop = true;
      }

      int alive = 0;
      bool stoped = true;
      for (int32_t i = 0; i < _maxThreads; i++) {
        if (_vctThreadPara[i]._bRunning.load(memory_order_relaxed)) {
          alive++;
        }
      }

      _aliveThreads = alive;
      if (alive == 0) {
        break;
      }
    }

    MList<ThreadTask *> queue;
    if (_queueTask.size() > 0) {
      unique_lock<SpinMutex> lock(_taskMutex);
      queue.swap(_queueTask);
    }
    _rapidTaskQueue.Pop(queue);

    if (_nowMicroSec - _checkBusyTime >= 10000) {
      CheckBusyStatus();
      _checkBusyTime = _nowMicroSec;

      if (_poolBusyDegree >= BusyDegree::BUSY && _aliveThreads < _maxThreads &&
          queue.size() > 0) {
        // If working threads are busy and new tasks coming and there still has
        // stoped threads, restart the stoped threads.
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
                 _aliveThreads - ThreadTask::GetExclusiveTaskCount() >
                     _minThreads) {
        // If the working threads are freed and its number is large than
        // _minThreads, select ont of freed thread to stop.
        double span = UINT64_MAX;
        int pos = -1;
        for (int32_t i = 0; i < _maxThreads; i++) {
          if (!_vctThreadPara[i]._bStop && !_vctThreadPara[i]._bExclusiveTask &&
              _vctThreadPara[i]._dtUsed < span) {
            span = _vctThreadPara[i]._dtUsed;
            pos = i;
          }
        }

        if (pos >= 0) {
          _vctThreadPara[pos]._bStop = true;
          _aliveThreads--;
        }
      }
    }

    // If there have a lot of waitting tasks and stoped threads, restart some of
    // free threads.
    if ((queue.size() >
             (_aliveThreads - ThreadTask::GetExclusiveTaskCount()) * 5 &&
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
        if (_aliveThreads < _maxThreads &&
            (_poolBusyDegree >= BusyDegree::BUSY ||
             _aliveThreads < ThreadTask::GetExclusiveTaskCount())) {
          assert(ThreadTask::GetExclusiveTaskCount() < _maxThreads);
          CreateWorkThread();
        }

        int32_t pos = -1;
        double span = UINT64_MAX;
        for (int32_t i = 0; i < _maxThreads; i++) {
          if (!_vctThreadPara[i]._bStop && !_vctThreadPara[i]._bExclusiveTask) {
            if (span > _vctThreadPara[i]._dtUsed) {
              pos = i;
              span = _vctThreadPara[i]._dtUsed;
            }
          }
        }

        assert(pos >= 0);
        _vctThreadPara[pos]._bExclusiveTask = true;
        _vctThreadPara[pos]._lineQueueTask.Push(task);
        LOG_INFO << "Add 1 exclusive " + task->GetTaskName() +
                        " task into thread "
                 << pos;
        _vctThreadPara[pos].ClearMask();
      } else {
        int32_t idx = 0;
        int posNorm = -1;
        int posConf = -1;
        int cnt = 0;
        double minNormSpan = UINT64_MAX;
        double minConfSpan = UINT64_MAX;
        double dtAvg = 0;

        while (idx < _maxThreads) {
          if (_vctThreadPara[idx]._bStop &&
                  !_stopThreads.load(memory_order_relaxed) ||
              _vctThreadPara[idx]._bExclusiveTask) {
            idx++;
            continue;
          }

          dtAvg += _vctThreadPara[idx]._dtUsed;
          cnt++;
          if (_vctThreadPara[idx].IsMaskConflict(task->GetTaskMask())) {
            if (_vctThreadPara[idx]._dtUsed < minConfSpan) {
              minConfSpan = _vctThreadPara[idx]._dtUsed;
              posConf = idx;
            }
            idx++;
            continue;
          }

          if (_vctThreadPara[idx]._dtUsed < minNormSpan) {
            minNormSpan = _vctThreadPara[idx]._dtUsed;
            posNorm = idx;
          }

          idx++;
        }

        dtAvg /= cnt;
        if (dtAvg == 0) {
          dtAvg = 10;
        }
        if (posNorm >= 0) {
          _vctThreadPara[posNorm]._lineQueueTask.Push(task);
          _vctThreadPara[posNorm].AddMask(task->GetTaskMask());
          _vctThreadPara[posNorm]._dtUsed += dtAvg;
          LOG_INFO << "Add normal task " + task->GetTaskName() + " into thread "
                   << posNorm;
        } else {
          assert(posConf >= 0);
          _vctThreadPara[posConf]._lineQueueTask.Push(task);
          _vctThreadPara[posConf].AddMask(task->GetTaskMask());
          _vctThreadPara[posConf]._dtUsed += dtAvg;
          LOG_INFO << "Add normal task " + task->GetTaskName() + " into thread "
                   << posConf;
        }
      }
    }

    this_thread::sleep_for(2us);
  }

  _nowMicroSec = 0;
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
    MList<ThreadTask *> lst;
    tpara._lineQueueTask.Pop(lst);
    if (lst.size() > 0) {
      if (lst.size() == 1 && lst.back()->IsExclusiveTask()) {
        AddTasks(GetThreadId(), tpara._vctTask);
      }

      tpara._vctTask.insert(tpara._vctTask.end(), lst.begin(), lst.end());
    }

    DT_MicroSec dtStart = _nowMicroSec;
    if (tpara._vctTask.size() > 0) {
      if (tpara._bExclusiveTask) {
        TaskStatus ts = tpara._vctTask[0]->Run();
        DT_MicroSec us = _nowMicroSec - dtStart;
        tpara._dtUsedTotal += us;
        tpara._runTimes++;

        if (ts == TaskStatus::FINISHED) {
          LOG_INFO << "Remove the finished exclusive task " +
                          tpara._vctTask[0]->GetTaskName() + " from thread "
                   << tid;
          if (tpara._vctTask[0]->IsNeedDelete()) {
            delete tpara._vctTask[0];
          }

          tpara._vctTask.clear();
          tpara._bExclusiveTask = false;
          tpara._dtUsed = 0;

        } else {
          tpara._vctTask[0]->SetAvgUsedTime(
              (tpara._vctTask[0]->GetAvgUsedTime() * 49 + us) / 50);
          tpara._dtUsed = (tpara._dtUsed * 49 + us) / 50;

          if (tpara._bStop) {
            if (IsStoped()) {
              continue;
            }

            AddTask(GetThreadId(), tpara._vctTask[0]);
            tpara._bExclusiveTask = false;
            tpara._vctTask.clear();

            LOG_INFO << "Remove the exclusive task " +
                            tpara._vctTask[0]->GetTaskName() +
                            " from stoped thread "
                     << tid;
            break;
          } else {
            continue;
          }
        }
      } else {
        for (auto iter = tpara._vctTask.begin();
             iter != tpara._vctTask.end();) {
          assert(!(*iter)->IsExclusiveTask());
          DT_MicroSec dtS = _nowMicroSec;
          TaskStatus ts = (*iter)->Run();
          if (ts == TaskStatus::FINISHED) {
            LOG_INFO << "Remove the finished normal task " +
                            (*iter)->GetTaskName() + " from thread "
                     << tid;
            if ((*iter)->IsNeedDelete()) {
              delete (*iter);
            }

            iter = tpara._vctTask.erase(iter);

          } else {
            (*iter)->SetAvgUsedTime(
                ((*iter)->GetAvgUsedTime() * 49 + _nowMicroSec - dtS) / 50);
            iter++;
          }
        }
      }
    }

    DT_MicroSec us = _nowMicroSec - dtStart;
    tpara._dtUsedTotal += us;
    tpara._runTimes++;
    tpara._dtUsed = (tpara._dtUsed * 49 + us) / 50;
    BusyDegree bd = CalcBusyDegree(tpara._dtUsed);

    if (tpara._bSelRmTask && bd >= BusyDegree::BUSY &&
        tpara._vctTask.size() > 1 && _aliveThreads < _maxThreads) {
      // This thread is busy and other threads is relax, move the smallest task
      // to other thread.
      auto iter = tpara._vctTask.begin();
      auto itSel = iter;
      iter++;

      for (; iter != tpara._vctTask.end(); iter++) {
        if ((*iter)->GetAvgUsedTime() < (*itSel)->GetAvgUsedTime()) {
          itSel = iter;
        }
      }

      AddTask(GetThreadId(), *itSel);
      tpara._vctTask.erase(itSel);
      tpara._bSelRmTask = false;
      LOG_INFO << "Remove a normal task " + (*itSel)->GetTaskName() +
                      " from busy thread "
               << tid << "  BusyDegree: " << (int)bd
               << "   Time: " << tpara._dtUsed
               << "  taskNum: " << tpara._vctTask.size();
    } else if (tpara._bStop) {
      if (tpara._vctTask.size() > 0) {
        if (IsStoped()) {
          continue;
        }

        LOG_INFO << "Remove " << tpara._vctTask.size()
                 << " normal tasks from stoped thread " << tid;
        AddTasks(GetThreadId(), tpara._vctTask);
      }

      break;
    }
  }

  tpara._bRunning.store(false, memory_order_release);
  tpara._thread->detach();
  delete tpara._thread;
  tpara._thread = nullptr;

  LOG_INFO << "Stop thread in thread pool, Name = " << _threadName;
}

void ThreadPool::PrintThreadTime() {
  for (size_t i = 0; i < _instMain->_vctThreadPara.size(); i++) {
    ThreadPara &para = _instMain->_vctThreadPara[i];
    LOG_INFO << i << ": " << para._runTimes << "\t" << para._dtUsedTotal;
    para._dtUsedTotal = 0;
    para._runTimes = 0;
  }
}
} // namespace storage
