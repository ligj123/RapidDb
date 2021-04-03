#include "../../src/utils/ThreadPool.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace utils {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ThreadPool_test) {
  ThreadPool tp("Test_ThreadPool", 3);
  future<string> rt1 = tp.AddTask([](string str) { return str; }, "aaa");

  BOOST_TEST(rt1.get() == "aaa");
  tp.Stop();

  try {
    future<int> rt2 = tp.AddTask([](int m) { return m; }, 3);
  } catch (runtime_error ex) {
    BOOST_TEST(ex.what(), "add task on stopped ThreadPool");
  }
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace utils