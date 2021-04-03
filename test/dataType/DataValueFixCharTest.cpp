#include "../../src/dataType/DataValueFixChar.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)
BOOST_AUTO_TEST_CASE(DataValueFixChar_test) {
  DataValueFixChar dv1;
  BOOST_TEST(dv1.GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength() == 1);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueFixChar dv2(100, true);
  BOOST_TEST(dv2.GetMaxLength() == 100);
  BOOST_TEST(dv2.GetDataLength() == 100);
  BOOST_TEST(dv2.GetPersistenceLength() == 100);
  DataValueFixChar dv3(100, true);
  BOOST_TEST(dv1 == dv2);
  BOOST_TEST(dv1 == dv3);

  const char *pStr = "abcd";
  DataValueFixChar dv4(pStr);
  BOOST_TEST(dv4.GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv4.IsNull());
  BOOST_TEST(dv4.GetDataLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv4.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv4.GetPersistenceLength() == DEFAULT_MAX_LEN + 1);
  BOOST_TEST(dv1 < dv4);
  BOOST_TEST(dv1 <= dv4);
  BOOST_TEST(dv1 != dv4);
  BOOST_TEST(std::any_cast<string>(dv4.GetValue()) == "abcd");

  dv2 = dv4;
  BOOST_TEST(dv4 == dv2);

  Byte buf[100] = "abcd";
  DataValueFixChar dv6(buf, 100, true);
  BOOST_TEST(std::any_cast<string>(dv6.GetValue()) == "abcd");
  BOOST_TEST(dv6.GetValueType() == ValueType::BYTES_VALUE);

  string str = std::any_cast<string>(dv6.GetValue());

  BOOST_TEST(dv6 > dv1);
  BOOST_TEST(dv6 >= dv1);
  BOOST_TEST(dv6 != dv1);

  dv1.SetDefaultValue();
  BOOST_TEST((string)dv1 == "");

  dv1.SetMaxValue();
  // BOOST_TEST((string)dv1 == "\\uff\\uff\\uff");

  dv1.SetMinValue();
  BOOST_TEST((string)dv1 == "");

  DataValueFixChar dv7;
  dv7.WriteData(buf);
  dv1.ReadData(buf, -1);
  BOOST_TEST(dv1 == dv7);

  DataValueFixChar dv9(pStr, 50);
  dv9.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv9);

  DataValueFixChar dv10(pStr, 50, true);
  dv10.WriteData(buf + 30);
  dv3.ReadData(buf + 30);
  BOOST_TEST(dv3 == dv10);

  DataValueFixChar *pDv = dv10.CloneDataValue();
  BOOST_TEST(pDv->GetValueType() == ValueType::NULL_VALUE);
  delete pDv;

  pDv = dv10.CloneDataValue(true);
  BOOST_TEST(pDv->GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST((string)(*pDv) == pStr);
  delete pDv;
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
