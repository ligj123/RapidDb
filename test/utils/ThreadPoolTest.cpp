#include "../../src/utils/ThreadPool.h"
#include "../../src/utils/Log.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ThreadPool_test) {
  class TestTask : public Task {
  public:
    TestTask() {}
    bool IsSmallTask() override { return false; }
    void Run() override {
      _val = ThreadPool::GetThreadId();
      LOG_INFO << "thread id: " << _val;
      this_thread::sleep_for(500ms);
      _status = TaskStatus::PAUSE_WITHOUT_ADD;
    }

  public:
    int _val = 0;
  };

  ThreadPool tp("Test_ThreadPool", 10000, 8, 8);
  TestTask arr[8];
  for (int i = 0; i < 8; i++) {
    tp.AddTask(&arr[i]);
    this_thread::sleep_for(1ms);
  }

  this_thread::sleep_for(1000ms);

  int count = 0;
  for (int i = 0; i < 8; i++) {
    count += arr[i]._val;
  }

  LOG_INFO << "count: " << count;
  BOOST_TEST(count == 28);
}

BOOST_AUTO_TEST_CASE(ThreadPoolDynamic_test) {
  class TestTask : public Task {
  public:
    TestTask(bool *pStop) : _pStop(pStop) {}
    bool IsSmallTask() override { return false; }
    void Run() override {
      while (!*_pStop) {
        this_thread::sleep_for(chrono::milliseconds(1));
      }
      _status = TaskStatus::FINISHED;
    }

  protected:
    bool *_pStop;
  };

  ThreadPool tp("Test_ThreadPool", 100000, 1, 8);
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());

  bool bStop = false;
  for (int i = 0; i < 20; i++) {
    tp.AddTask(new TestTask(&bStop));
    this_thread::sleep_for(1ms);
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  BOOST_TEST(tp.GetMaxThreads() == tp.GetThreadCount());

  bStop = true;
  this_thread::sleep_for(chrono::milliseconds(1000));
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());

  bStop = false;
  for (int i = 0; i < 20; i++) {
    tp.AddTask(new TestTask(&bStop));
    this_thread::sleep_for(1ms);
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  BOOST_TEST(tp.GetMaxThreads() == tp.GetThreadCount());

  bStop = true;
  this_thread::sleep_for(chrono::milliseconds(1000));
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
