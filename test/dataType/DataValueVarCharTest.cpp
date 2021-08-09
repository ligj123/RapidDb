#include "../../src/dataType/DataValueVarChar.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)

BOOST_AUTO_TEST_CASE(DataValueVarChar_test) {
  DataValueVarChar dv1;
  BOOST_TEST(dv1.GetDataType() == DataType::VARCHAR);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(!dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength() == 1);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueVarChar dv2(100, true);
  BOOST_TEST(dv2.GetMaxLength() == 100);
  BOOST_TEST(dv2.GetDataLength() == 0);
  BOOST_TEST(dv2.GetPersistenceLength() == 0);
  BOOST_TEST(dv1 == dv2);

  const char *pStr = "abcd";
  DataValueVarChar dv4(pStr, strlen(pStr));
  BOOST_TEST(dv4.GetDataType() == DataType::VARCHAR);
  BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv4.IsNull());
  BOOST_TEST(dv4.GetDataLength() == 5);
  BOOST_TEST(dv4.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv4.GetPersistenceLength() == 10);
  BOOST_TEST(dv1 < dv4);
  BOOST_TEST(dv1 <= dv4);
  BOOST_TEST(dv1 != dv4);
  BOOST_TEST(std::any_cast<string>(dv4.GetValue()) == "abcd");

  dv2 = dv4;
  BOOST_TEST(dv4 == dv2);

  DataValueVarChar dv5(pStr, 4, 100, true);
  BOOST_TEST(dv5.GetDataLength() == 5);
  BOOST_TEST(dv5.GetMaxLength() == 100);
  BOOST_TEST(dv5.GetPersistenceLength() == 5);

  Byte buf[100] = "abcd";
  DataValueVarChar dv6(buf, 4, 100, true);
  BOOST_TEST(std::any_cast<string>(dv6.GetValue()) == "abcd");
  BOOST_TEST(dv6.GetValueType() == ValueType::BYTES_VALUE);

  BOOST_TEST(dv6 > dv1);
  BOOST_TEST(dv6 >= dv1);
  BOOST_TEST(dv6 != dv1);

  dv1.SetDefaultValue();
  BOOST_TEST((string)dv1 == "");

  dv1.SetMaxValue();
  // BOOST_TEST((string)dv1 == "\\uff\\uff\\uff");

  dv1.SetMinValue();
  BOOST_TEST((string)dv1 == "");

  DataValueVarChar dv7;
  dv7.WriteData(buf);
  dv1.ReadData(buf, -1);
  BOOST_TEST(dv1 == dv7);

  DataValueVarChar dv8(true);
  dv8.WriteData(buf + 10);
  dv2.ReadData(buf + 10);
  BOOST_TEST(dv2 == dv8);

  DataValueVarChar dv9(pStr, strlen(pStr));
  dv9.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv9);

  DataValueVarChar dv10(pStr, 4, 100, true);
  dv10.WriteData(buf + 30);
  dv5.ReadData(buf + 30, dv10.GetDataLength());
  BOOST_TEST(dv5 == dv10);

  StrBuff sb(10);
  dv1 = "abcdefghijklmn1234567890";
  dv1.ToString(sb);
  BOOST_TEST(strcmp(sb.GetBuff(), "abcdefghijklmn1234567890") == 0);
  BOOST_TEST(sb.GetBufLen() > 24U);
  BOOST_TEST(sb.GetStrLen() == 24U);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
