#include "../../src/dataType/DataValueDate.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)

BOOST_AUTO_TEST_CASE(DataValueDate_test) {
  DataValueDate dv1;
  BOOST_TEST(dv1.GetDataType() == DataType::DATETIME);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == 8);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength() == 0);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueDate dv2(true);
  BOOST_TEST(dv2.GetMaxLength() == 8);
  BOOST_TEST(dv2.GetDataLength() == 8);
  BOOST_TEST(dv2.GetPersistenceLength() == 8);
  BOOST_TEST(dv1 == dv2);

  DataValueDate dv3(2, false);
  BOOST_TEST(dv3.GetDataType() == DataType::DATETIME);
  BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv3.IsNull());
  BOOST_TEST(dv3.GetDataLength() == 8);
  BOOST_TEST(dv3.GetMaxLength() == 8);
  BOOST_TEST(dv3.GetPersistenceLength() == 8);
  BOOST_TEST(dv1 < dv3);
  BOOST_TEST(dv1 <= dv3);
  BOOST_TEST(dv1 != dv3);
  BOOST_TEST(std::any_cast<uint64_t>(dv3.GetValue()) == 2);

  DataValueDate dv4(2, true);
  BOOST_TEST(dv4.GetPersistenceLength() == 8);
  BOOST_TEST(dv4 == dv3);

  Byte buf[100];
  DataValueDate dv5(10, false);
  BOOST_TEST(std::any_cast<uint64_t>(dv5.GetValue()) == 10);
  BOOST_TEST(dv5.GetValueType() == ValueType::SOLE_VALUE);

  BOOST_TEST(dv5 > dv3);
  BOOST_TEST(dv5 >= dv3);
  BOOST_TEST(dv5 != dv3);

  Byte buf2[100];
  uint32_t len = dv5.WriteData(buf2);
  BOOST_TEST(len == 8);
  BOOST_TEST(*((uint64_t *)buf2) == 10);

  dv1.SetDefaultValue();
  BOOST_TEST((uint64_t)dv1 == 0L);

  dv1.SetMaxValue();
  BOOST_TEST((uint64_t)dv1 == ULLONG_MAX);

  dv1.SetMinValue();
  BOOST_TEST((uint64_t)dv1 == 0);

  DataValueDate dv6(0x1234567890, true);
  dv6.WriteData(buf);
  dv2.ReadData(buf, 8);
  BOOST_TEST(dv2 == dv6);

  dv6 = 0x1234LL;
  dv6.WriteData(buf);
  dv2.ReadData(buf, 8);
  BOOST_TEST(std::any_cast<uint64_t>(dv2.GetValue()) == 0x1234);

  DataValueDate dv7(false);
  dv7.WriteData(buf + 20);
  dv1.ReadData(buf + 20, 0);
  BOOST_TEST(dv1 == dv7);

  dv7 = 10000LL;
  dv7.WriteData(buf + 20);
  dv1.ReadData(buf + 20, 8);
  BOOST_TEST(dv1 == dv7);

  DataValueDate dv8(dv7);
  BOOST_TEST(dv8 == dv7);

  dv6 = dv7;
  BOOST_TEST(dv6 == dv7);

  DataValueDate dv9(std::any(100ULL), false);
  BOOST_TEST((uint64_t)dv9 == 100ULL);

  StrBuff sb(0);
  dv1 = 1234567890LL;
  dv1.ToString(sb);
  BOOST_TEST(strcmp(sb.GetBuff(), "1234567890") == 0);
  BOOST_TEST(sb.GetBufLen() > 10U);
  BOOST_TEST(sb.GetStrLen() == 10U);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
