#pragma once

#include "../cache/Mallocator.h"
#include "ErrorMsg.h"
#include "SpinMutex.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>

namespace storage {
using namespace std;
const uint64_t UNLIMIT_CIRCLE = UINT64_MAX; // Unlimit repeated to circle
enum class TimerType : Byte {
  TIMING, // Only run one time at the pointed time.
  CIRCLE  // Repeated to run with fixed time gap
};

struct TimerTask {
  string _name;
  TimerType type;
  union {
    struct { // Repeated times
      uint64_t rpCount;
      // Interval between two neighbor running, time unit: milliseconds
      DT_MicroSec interval;
    };
    // The time(microseconds since epoch) to start this task
    DT_MicroSec dtStart;
  };
  // To save the privious time to run this task
  DT_MicroSec _prevTime = 0;
  // This function is used to generate Task and add into queue in thread pool
  std::function<void()> _lambda;
};

/**This thread will start when this process start and stop when this process
 * stop*/
class TimerThread {
public:
  TimerThread();
  ~TimerThread();
  static void AddCircleTask(string name, DT_MicroSec interval,
                            std::function<void()> lambda,
                            uint64_t rpCount = UNLIMIT_CIRCLE);
  static void AddTimingTask(string name, DT_MicroSec dtStart,
                            std::function<void()> lambda);
  static bool RemoveTask(string name);
  static DT_MicroSec GetCurrTime() {
    if (_instance == nullptr || !_instance->_bRunning)
      return 0;
    return _instance->_currTime;
  }
  static void Start();
  static void Stop();

protected:
  void Run();

protected:
  static TimerThread *_instance;
  DT_MicroSec _currTime;
  thread *_thread;
  MVector<TimerTask *>::Type _vctTask;
  bool _bRunning;
  SpinMutex _mutex;
};
} // namespace storage