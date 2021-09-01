﻿#include "../../src/dataType/DataValueDigit.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)

BOOST_AUTO_TEST_CASE(DataValueUInt_test) {
  DataValueUInt dv1;

  BOOST_TEST(dv1.GetDataType() == DataType::UINT);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == 4);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength() == 1);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueUInt dv2(true);
  BOOST_TEST(dv2.GetMaxLength() == 4);
  BOOST_TEST(dv2.GetDataLength() == 4);
  BOOST_TEST(dv2.GetPersistenceLength() == 4);
  BOOST_TEST(dv1 == dv2);

  DataValueUInt dv3(2, false);
  BOOST_TEST(dv3.GetDataType() == DataType::UINT);
  BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv3.IsNull());
  BOOST_TEST(dv3.GetDataLength() == 4);
  BOOST_TEST(dv3.GetMaxLength() == 4);
  BOOST_TEST(dv3.GetPersistenceLength() == 5);
  BOOST_TEST(dv1 < dv3);
  BOOST_TEST(dv1 <= dv3);
  BOOST_TEST(dv1 != dv3);
  BOOST_TEST(std::any_cast<uint32_t>(dv3.GetValue()) == 2);

  DataValueUInt dv4(2, true);
  BOOST_TEST(dv4.GetPersistenceLength() == 4);
  BOOST_TEST(dv4 == dv3);

  Byte buf[100];
  DataValueUInt dv5(10, false);
  BOOST_TEST(std::any_cast<uint32_t>(dv5.GetValue()) == 10);
  BOOST_TEST(dv5.GetValueType() == ValueType::SOLE_VALUE);

  BOOST_TEST(dv5 > dv3);
  BOOST_TEST(dv5 >= dv3);
  BOOST_TEST(dv5 != dv3);

  Byte buf2[100];
  uint32_t len = dv5.WriteData(buf2);
  BOOST_TEST(len == 5);
  BOOST_TEST(buf2[0] == 0x87);
  BOOST_TEST(*((uint32_t *)(buf2 + 1)) == 10);

  dv1.SetDefaultValue();
  BOOST_TEST((uint32_t)dv1 == 0L);

  dv1.SetMaxValue();
  BOOST_TEST((uint32_t)dv1 == UINT_MAX);

  dv1.SetMinValue();
  BOOST_TEST((uint32_t)dv1 == 0);

  DataValueUInt dv6(0x12345678, true);
  dv6.WriteData(buf);
  dv2.ReadData(buf, -1);
  BOOST_TEST(dv2 == dv6);

  dv6 = 0x1234;
  dv6.WriteData(buf);
  dv2.ReadData(buf);
  BOOST_TEST(std::any_cast<uint32_t>(dv2.GetValue()) == 0x1234);

  DataValueUInt dv7(false);
  dv7.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv7);

  dv7 = 10000;
  dv7.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv7);

  DataValueUInt dv8(dv7);
  BOOST_TEST(dv8 == dv7);

  dv6 = dv7;
  BOOST_TEST(dv6 == dv7);

  DataValueUInt dv9(std::any(100), false);
  BOOST_TEST((int64_t)dv9 == 100);

  StrBuff sb(0);
  dv1 = 1234567890;
  dv1.ToString(sb);
  BOOST_TEST(strcmp(sb.GetBuff(), "1234567890") == 0);
  BOOST_TEST(sb.GetBufLen() > 10U);
  BOOST_TEST(sb.GetStrLen() == 10U);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
