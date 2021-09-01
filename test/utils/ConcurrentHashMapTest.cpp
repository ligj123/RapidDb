#include "../../src/utils/ConcurrentHashMap.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ConcurrentHashMap_test) {
  class ConcurrentHashMapEx : public ConcurrentHashMap<int, string> {
  public:
    using ConcurrentHashMap::_groupCount;
    using ConcurrentHashMap::_vctLock;
    using ConcurrentHashMap::_vctMap;
    using ConcurrentHashMap::ConcurrentHashMap;
  };

  ConcurrentHashMapEx hMap(100, 1000000);
  BOOST_TEST(hMap._groupCount == 100);
  BOOST_TEST(hMap._vctMap.size() == 100);
  BOOST_TEST(hMap._vctLock.size() = 100);
  BOOST_TEST(hMap._vctMap[0]->size() == 0);
  BOOST_TEST(hMap._vctMap[0]->bucket_count() >= 10000);

  for (int i = 0; i < 500000; i++) {
    BOOST_TEST(hMap.insert(i, to_string(i)));
  }

  BOOST_TEST(hMap.size() == 500000);

  for (int i = 0; i < 500000; i++) {
    string str;
    BOOST_TEST(hMap.find(i, str));
    BOOST_TEST(str == to_string(i));
  }

  int count = 0;
  for (int i = 0; i < 100; i++) {
    auto iter = hMap.begin(i);
    auto end = hMap.end(i);
    while (iter != end) {
      count++;
      iter++;
    }
    hMap.unlock(i);
  }

  BOOST_TEST(count == 500000);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
