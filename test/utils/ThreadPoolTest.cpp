#include "../../src/utils/ThreadPool.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ThreadPool_test) {
  class TestTask : public Task {
  public:
    TestTask(int val) : _val(val) {}
    void Run() override { _promise.set_value(_val); }

  protected:
    int _val;
  };
  ThreadPool tp("Test_ThreadPool");
  Task *task = new TestTask(100);
  future<int> ft = task->GetFuture();
  tp.AddTask(task);

  BOOST_TEST(ft.get() == 100);
  tp.Stop();

  try {
    tp.AddTask(new TestTask(10));
  } catch (runtime_error ex) {
    BOOST_TEST(ex.what(), "add task on stopped ThreadPool");
  }
}

BOOST_AUTO_TEST_CASE(ThreadPoolDynamic_test) {
  class TestTask : public Task {
  public:
    TestTask(bool *pStop) : _pStop(pStop) {}
    void Run() override {
      while (!*_pStop) {
        this_thread::sleep_for(chrono::milliseconds(1));
      }
    }

  protected:
    bool *_pStop;
  };

  ThreadPool tp("Test_ThreadPool");
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());

  bool bStop = false;
  for (int i = 0; i < 20; i++) {
    tp.AddTask(new TestTask(&bStop));
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  BOOST_TEST(tp.GetMaxThreads() == tp.GetThreadCount());

  bStop = true;
  this_thread::sleep_for(chrono::milliseconds(1000));
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());
  BOOST_TEST(tp.GetCurrId() == 8);

  bStop = false;
  for (int i = 0; i < 20; i++) {
    tp.AddTask(new TestTask(&bStop));
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  BOOST_TEST(tp.GetMaxThreads() == tp.GetThreadCount());

  bStop = true;
  this_thread::sleep_for(chrono::milliseconds(1000));
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());
  BOOST_TEST(tp.GetCurrId() == 15);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
