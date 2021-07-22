#include "../../src/utils/ThreadPool.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace utils {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ThreadPool_test) {
  ThreadPool tp("Test_ThreadPool");
  future<string> rt1 = tp.AddTask([](string str) { return str; }, "aaa");

  BOOST_TEST(rt1.get() == "aaa");
  tp.Stop();

  try {
    future<int> rt2 = tp.AddTask([](int m) { return m; }, 3);
  } catch (runtime_error ex) {
    BOOST_TEST(ex.what(), "add task on stopped ThreadPool");
  }
}

BOOST_AUTO_TEST_CASE(ThreadPoolDynamic_test) {
  ThreadPool tp("Test_ThreadPool");
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());

  bool bStop = false;
  for (int i = 0; i < 20; i++) {
    tp.AddTask([&bStop]() {
      while (!bStop) {
        this_thread::sleep_for(chrono::milliseconds(1));
      }
    });
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  BOOST_TEST(tp.GetMaxThreads() == tp.GetThreadCount());

  bStop = true;
  this_thread::sleep_for(chrono::milliseconds(1000));
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());
  BOOST_TEST(tp.GetCurrId() == tp.GetMinThreads());

  bStop = false;
  for (int i = 0; i < 20; i++) {
    tp.AddTask([&bStop]() {
      while (!bStop) {
        this_thread::sleep_for(chrono::milliseconds(1));
      }
    });
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  BOOST_TEST(tp.GetMaxThreads() == tp.GetThreadCount());

  bStop = true;
  this_thread::sleep_for(chrono::milliseconds(1000));
  BOOST_TEST(tp.GetMinThreads() == tp.GetThreadCount());
  BOOST_TEST(tp.GetCurrId() == tp.GetMinThreads() * 2 - 1);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace utils
