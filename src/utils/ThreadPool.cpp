#include "ThreadPool.h"
#include "FastQueue.h"
#include "Log.h"
#include "Utilitys.h"

#include <stdexcept>

namespace storage {
// The default thread id is -1 expected threads from pool.
thread_local string ThreadPool::_threadName = "main";
thread_local int ThreadPool::_threadID = -1;

ThreadPool::ThreadPool(string threadPrefix, uint32_t maxQueueSize,
                       int minThreads, int maxThreads)
    : _threadPrefix(threadPrefix), _maxQueueSize(maxQueueSize),
      _minThreads(minThreads), _maxThreads(maxThreads) {
  assert(_minThreads >= 1 && _minThreads <= _maxThreads);
  _vctThreadPara.resize(maxThreads);

  for (int i = 0; i < minThreads; ++i) {
    CreateThread();
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

  _vctThreadPara[id]._thread = thread([this, id]() {
    _threadName = _threadPrefix + "_" + to_string(id);
    assert(_threadName.size() <= 15);
    _threadID = id;
#ifdef LINUX_OS
    pthread_setname_np(pthread_self(), _threadName.c_str());
#endif
    ThreadPara &tpara = _vctThreadPara[id];
    tpara._bRunning = true;
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
        MVector<ThreadTask *> tmp;
        tmp.reserve(tpara._vctTask.size());
        for (auto iter = tpara._vctTask.begin();
             iter != tpara._vctTask.end();) {
          TaskStatus ts = (*iter)->Run();
          if (ts == TaskStatus::FINISHED) {
            if ((*iter)->IsNeedFree()) {
              delete (*iter);
            }

            iter = tpara._vctTask.erase(iter);
          } else if (ts == TaskStatus::INTERVAL) {
            tmp.push_back(*iter);
            iter = tpara._vctTask.erase(iter);
          } else {
            iter++;
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

        if ((tpara._busyDegree == BusyDegree::BLOCKED &&
                 tpara._repeatTime >= 3 ||
             tpara._busyDegree == BusyDegree::BUSY && tpara._repeatTime >= 5) &&
            tpara._vctTask.size() > 1) {
          auto iter = tpara._vctTask.begin();
          auto itSel = iter;
          iter++;

          for (; iter != tpara._vctTask.end(); iter++) {
            if ((*iter)->GetBusyDegree() < (*itSel)->GetBusyDegree()) {
              itSel = iter;
            }
          }

          tmp.push_back(*itSel);
        }

        if (tmp.size() > 0) {
          AddTasks(tmp);
        }
      }

      if (tpara._busyDegree >= BusyDegree::BUSY &&
          (_queueTask.size() == 0 || (MicroSecTime() - _taskTime < 10000))) {
        continue;
      }

      if (_queueTask.size() == 0) {
        if (tpara._busyDegree == BusyDegree::EMPTY) {
          if (tpara._repeatTime < 3) {
            std::unique_lock<SpinMutex> queue_lock(_task_mutex);
            _taskCv.wait_for(queue_lock, 10ms, [this]() -> bool {
              return _queueTask.size() > 0 || _stopThreads;
            });
          } else {
            tpara._bRunning = false;
            break;
          }
        } else {
          std::unique_lock<SpinMutex> queue_lock(_task_mutex);
          _taskCv.wait_for(queue_lock, 10ms, [this]() -> bool {
            return _queueTask.size() > 0 || _stopThreads;
          });

          if (_queueTask.size() == 0)
            continue;
        }
      }

      std::unique_lock<SpinMutex> queue_lock(_task_mutex);
      ThreadTask *task = _queueTask.front();
      _queueTask.pop_front();
      tpara._vctTask.push_back(task);
    }
  });
}

ThreadPool::~ThreadPool() {
  for (int i = 0; i < _maxThreads; i++) {
    ThreadPara &tpara = _vctThreadPara[i];
    if (!tpara._bRunning)
      continue;

    tpara._thread.join();
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
} // namespace storage
