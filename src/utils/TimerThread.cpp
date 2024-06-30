#include "TimerThread.h"
#include "ThreadPool.h"

namespace storage {
TimerThread *TimerThread::_instance = nullptr;

TimerThread::TimerThread() : _currTime(0), _bRunning(false) {
  _thread = new thread([this]() { Run(); });
}

TimerThread::~TimerThread() {
  _thread->join();
  delete _thread;

  for (TimerTask *task : _vctTask) {
    delete task;
  }
}

void TimerThread::Run() {
  _bRunning = true;
  // TimeThread do not use FastQueue, so its id set to negitive.
  ThreadPool::AddThread("TimerThread", -1);

  while (_bRunning) {
    _currTime = chrono::duration_cast<chrono::microseconds>(
                    chrono::system_clock::now().time_since_epoch())
                    .count();

    if (_mutex.try_lock()) {
      for (size_t i = 0; i < _vctTask.size();) {
        TimerTask *task = _vctTask[i];
        if (task->type == TimerType::TIMING) {
          if (task->dtStart <= _currTime) {
            task->_lambda();
            _vctTask.erase(_vctTask.begin() + i);
            continue;
          }
        } else {
          if (task->_prevTime + task->interval <= _currTime) {
            task->_lambda();
            if (task->rpCount != UNLIMIT_CIRCLE) {
              task->rpCount--;
              if (task->rpCount == 0) {
                _vctTask.erase(_vctTask.begin() + i);
                continue;
              }
            }

            task->_prevTime = _currTime;
          }
        }
        i++;
      }
      _mutex.unlock();
    }

    this_thread::sleep_for(chrono::microseconds(10));
  }

  ThreadPool::RemoveThread(-1);
}

void TimerThread::AddCircleTask(string name, DT_MicroSec interval,
                                std::function<void()> lambda, bool memTask,
                                uint64_t rpCount) {
  unique_lock<SpinMutex> lock(_instance->_mutex);
  TimerTask *task = new TimerTask;
  task->_name = name;
  task->type = TimerType::CIRCLE;
  task->rpCount = rpCount;
  task->interval = interval;
  task->_lambda = lambda;
  task->_bMemTask = memTask;
  _instance->_vctTask.push_back(task);
}

void TimerThread::AddTimingTask(string name, DT_MicroSec dtStart,
                                std::function<void()> lambda) {
  unique_lock<SpinMutex> lock(_instance->_mutex);
  TimerTask *task = new TimerTask;
  task->_name = name;
  task->type = TimerType::TIMING;
  task->dtStart = dtStart;
  task->_lambda = lambda;
  _instance->_vctTask.push_back(task);
}

bool TimerThread::RemoveTask(string name) {
  unique_lock<SpinMutex> lock(_instance->_mutex);
  for (size_t i = 0; i < _instance->_vctTask.size(); i++) {
    TimerTask *task = _instance->_vctTask[i];
    if (task->_name == name) {
      _instance->_vctTask.erase(_instance->_vctTask.begin() + i);
      return true;
    }
  }

  return true;
}

void TimerThread::Start() {
  assert(_instance == nullptr || !_instance->_bRunning);
  if (_instance == nullptr) {
    _instance = new TimerThread();
  } else {
    delete _instance;
    _instance = new TimerThread();
  }
}

void TimerThread::Stop() {
  assert(_instance != nullptr && _instance->_bRunning);
  _instance->_bRunning = false;

  delete _instance;
  _instance = nullptr;
}

void TimerThread::TriggerMemTask() {
  unique_lock<SpinMutex> lock(_instance->_mutex);
  for (TimerTask *task : _instance->_vctTask) {
    if (!task->_bMemTask)
      continue;

    task->_lambda();
  }
}
} // namespace storage