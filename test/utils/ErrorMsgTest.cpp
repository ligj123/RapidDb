#include "../../src/utils/ErrorMsg.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ErrorMsg_test) {
  ErrorMsg err1(1);
  BOOST_TEST(err1.what(), "function test");

  ErrorMsg err2(2, {"e1", "e2"});
  BOOST_TEST(err2.what(), "table error1-e1, error2-e2");
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
