#include "../../src/utils/FastQueue.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/utils/ThreadPool.h"
#include <boost/test/unit_test.hpp>
#include <string>
using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(FastQueue_test) {
  static atomic_int64_t taskCount{0};
  static FastQueue<DataValueLong> fastQueue(8);
  static bool bStop1 = false;
  static bool bStop2 = false;

  class TestTask : public Task {
  public:
    TestTask() {}
    bool IsSmallTask() override { return false; }
    void Run() override {
      while (!bStop1) {
        fastQueue.Push(
            new DataValueLong(taskCount.fetch_add(1, memory_order_relaxed)));
        this_thread::sleep_for(chrono::milliseconds(1));
      }

      _status = TaskStatus::STOPED;
    }
  };

  ThreadPool tp("Test_ThreadPool", 100000, 1, 8);
  for (int i = 0; i < 8; i++) {
    tp.AddTask(new TestTask());
  }

  static int64_t maxVal = 0;
  static int64_t count = 0;

  thread t([]() {
    while (!bStop2) {
      queue<DataValueLong *> q;
      fastQueue.Swap(q);

      while (q.size() > 0) {
        DataValueLong *dv = q.front();
        if ((int64_t)(*dv) > maxVal)
          maxVal = *dv;
        delete dv;
        count++;
        q.pop();
      }

      DataValueLong *dv = fastQueue.Pop();
      if (dv != nullptr) {
        if ((int64_t)(*dv) > maxVal)
          maxVal = *dv;
        delete dv;
        count++;
      }
    }
  });

  this_thread::sleep_for(1000ms);
  bStop1 = true;
  tp.Stop();

  this_thread::sleep_for(100ms);
  bStop2 = true;
  t.join();

  int64_t val = taskCount.load(memory_order_acquire);
  BOOST_TEST(maxVal = count - 1);
  BOOST_TEST(val == count);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
