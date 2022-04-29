#include "TimerThread.h"

namespace storage {
TimerThread *TimerThread::_instance = new TimerThread();

TimerThread::TimerThread() {
  _thread = new thread([this]() { Run(); });
}

TimerThread::~TimerThread() {
  for (TimerTask *task : _vctTask) {
    delete task;
  }
}

void TimerThread::Run() {
  while (true) {
    _currTime = chrono::duration_cast<chrono::microseconds>(
                    chrono::system_clock::now().time_since_epoch())
                    .count();

    for (size_t i = 0; i < _vctTask.size();) {
      TimerTask *task = _vctTask[i];
      if (task->type == TimerType::CIRCLE) {
        if (_currTime >= task->dtStart) {
          task->_lambda();
          _vctTask.erase(_vctTask.begin() + i);
          continue;
        }
      } else {
        if (task->_prevTime + task->interval >= _currTime) {
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
    this_thread::sleep_for(chrono::microseconds(1));
  }
}

void TimerThread::AddCircleTask(string name, DT_MicroSec interval,
                                std::function<void()> lambda,
                                uint64_t rpCount) {
  TimerTask *task = new TimerTask;
  task->_name = name;
  task->type = TimerType::CIRCLE;
  task->rpCount = rpCount;
  task->interval = interval;
  task->_lambda = lambda;
  _instance->_vctTask.push_back(task);
}

void TimerThread::AddTimingTask(string name, DT_MicroSec dtStart,
                                std::function<void()> lambda) {
  TimerTask *task = new TimerTask;
  task->_name = name;
  task->type = TimerType::TIMING;
  task->dtStart = dtStart;
  task->_lambda = lambda;
  _instance->_vctTask.push_back(task);
}

bool TimerThread::RemoveTask(string name) {
  for (size_t i = 0; i < _instance->_vctTask.size();) {
    TimerTask *task = _instance->_vctTask[i];
    if (task->_name == name) {
      _instance->_vctTask.erase(_instance->_vctTask.begin() + i);
      return true;
    }
  }

  return true;
}

} // namespace storage