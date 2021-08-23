#include "../../src/dataType/DataValueDigit.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)

BOOST_AUTO_TEST_CASE(DataValueChar_test) {
  DataValueChar dv1;

  BOOST_TEST(dv1.GetDataType() == DataType::CHAR);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == 1);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength() == 1);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueChar dv2(true);
  BOOST_TEST(dv2.GetMaxLength() == 1);
  BOOST_TEST(dv2.GetDataLength() == 1);
  BOOST_TEST(dv2.GetPersistenceLength() == 1);
  BOOST_TEST(dv1 == dv2);

  DataValueChar dv3(2, false);
  BOOST_TEST(dv3.GetDataType() == DataType::CHAR);
  BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv3.IsNull());
  BOOST_TEST(dv3.GetDataLength() == 1);
  BOOST_TEST(dv3.GetMaxLength() == 1);
  BOOST_TEST(dv3.GetPersistenceLength() == 2);
  BOOST_TEST(dv1 < dv3);
  BOOST_TEST(dv1 <= dv3);
  BOOST_TEST(dv1 != dv3);
  BOOST_TEST(std::any_cast<char>(dv3.GetValue()) == 2);

  DataValueChar dv4(2, true);
  BOOST_TEST(dv4.GetPersistenceLength() == 1);
  BOOST_TEST(dv4 == dv3);

  Byte buf[100];
  DataValueChar dv5(10, false);
  BOOST_TEST(std::any_cast<char>(dv5.GetValue()) == 10);
  BOOST_TEST(dv5.GetValueType() == ValueType::SOLE_VALUE);

  BOOST_TEST(dv5 > dv3);
  BOOST_TEST(dv5 >= dv3);
  BOOST_TEST(dv5 != dv3);

  Byte buf2[100];
  uint32_t len = dv5.WriteData(buf2);
  BOOST_TEST(len == 2);
  BOOST_TEST(buf2[0] == 0x81);
  BOOST_TEST(*((int8_t *)(buf2 + 1)) == 10);

  dv1.SetDefaultValue();
  BOOST_TEST((int8_t)dv1 == 0L);

  dv1.SetMaxValue();
  BOOST_TEST((int8_t)dv1 == CHAR_MAX);

  dv1.SetMinValue();
  BOOST_TEST((int8_t)dv1 == CHAR_MIN);

  DataValueChar dv6('a', true);
  dv6.WriteData(buf);
  dv2.ReadData(buf, -1);
  BOOST_TEST(dv2 == dv6);

  dv6 = 0x12;
  dv6.WriteData(buf);
  dv2.ReadData(buf);
  BOOST_TEST(std::any_cast<char>(dv2.GetValue()) == 0x12);

  DataValueChar dv7(false);
  dv7.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv7);

  dv7 = 100;
  dv7.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv7);

  DataValueChar dv8(dv7);
  BOOST_TEST(dv8 == dv7);

  dv6 = dv7;
  BOOST_TEST(dv6 == dv7);

  DataValueChar dv9(std::any(100));
  BOOST_TEST((int64_t)dv9 == 100);

  StrBuff sb(0);
  dv1 = 99;
  dv1.ToString(sb);
  BOOST_TEST(strcmp(sb.GetBuff(), "99") == 0);
  BOOST_TEST(sb.GetBufLen() > 2U);
  BOOST_TEST(sb.GetStrLen() == 2U);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
