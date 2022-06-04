#include "../src/utils/SpinMutex.h"
#include "PressTest.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>


namespace storage {
using namespace std;
static int64_t times = 100000000;

void MutexTest() {
  ReentrantSharedSpinMutex ssm;
  SpinMutex sm;
  std::mutex mu;
  chrono::system_clock::time_point st = chrono::system_clock::now();

  for (int i = 0; i < times; i++) {
    std::lock_guard<SpinMutex> lock(sm);
  }

  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  cout << "SpinMutex Time(ms):" << duration.count() << endl;

  st = chrono::system_clock::now();

  for (int i = 0; i < times; i++) {
    std::lock_guard<mutex> lock(mu);
  }

  et = chrono::system_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  cout << "std::mutex Time(ms):" << duration.count() << endl;
}
} // namespace storage