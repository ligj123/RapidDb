#include "../../src/utils/ConcurrentHashMap.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(ConcurrentHashMap_test) {
  class ConcurrentHashMapEx : public ConcurrentHashMap<int, MString, false> {
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

  for (int i = 0; i < 50000; i++) {
    BOOST_TEST(hMap.Insert(i, ToMString(i)));
  }

  BOOST_TEST(hMap.Size() == 50000);

  for (int i = 0; i < 50000; i++) {
    MString str;
    BOOST_TEST(hMap.Find(i, str));
    BOOST_TEST(str == ToMString(i));
  }

  int count = 0;
  for (int i = 0; i < 100; i++) {
    hMap.Lock(i);
    auto iter = hMap.Begin(i);
    auto end = hMap.End(i);
    while (iter != end) {
      count++;
      iter++;
    }
    hMap.Unlock(i);
  }

  BOOST_TEST(count == 50000);
  hMap.Clear();
}

BOOST_AUTO_TEST_CASE(ConcurrentHashMap_UseFunc_test) {
  int64_t count = 0;
  ConcurrentHashMap<int, int, false> hMap(
      100, 1000000, [&count](int ii) { count += ii; },
      [&count](int ii) { count -= ii; });

  for (int i = 0; i < 50000; i++) {
    BOOST_TEST(hMap.Insert(i, i));
  }

  for (int i = 0; i < 50000; i++) {
    int num;
    BOOST_TEST(hMap.Find(i, num));
    BOOST_TEST(num == i);
  }

  BOOST_TEST(count == (1 + 50000) * 50000LL / 2);

  for (int i = 0; i < 1000; i++) {
    int num = i;
    BOOST_TEST(hMap.Erase(num));
  }

  hMap.Clear();
  BOOST_TEST(count == 0);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
