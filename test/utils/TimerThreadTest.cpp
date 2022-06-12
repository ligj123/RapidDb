#include "../../src/utils/TimerThread.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(TimerThread_test) {
  TimerThread::Start();
  int count = 0;
  TimerThread::AddCircleTask(
      "circle_test", 1000, [&count]() { count++; }, false, 10);
  this_thread::sleep_for(1s);
  BOOST_TEST(count == 10);
  BOOST_TEST(TimerThread::GetTaskCount() == 0);

  uint64_t ts = chrono::duration_cast<chrono::microseconds>(
                    chrono::system_clock::now().time_since_epoch())
                    .count() +
                100;
  TimerThread::AddTimingTask("timer_test", ts, [&count]() { count = 100; });
  BOOST_TEST(TimerThread::GetTaskCount() == 1);
  this_thread::sleep_for(1s);
  BOOST_TEST(count == 100);
  BOOST_TEST(TimerThread::GetTaskCount() == 0);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
