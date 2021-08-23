#include "../../src/dataType/DataValueFixChar.h"
#include "../../src/dataType/DataValueDigit.h"
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
  DataValueFixChar dv4(pStr, 4, 10);
  BOOST_TEST(dv4.GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv4.IsNull());
  BOOST_TEST(dv4.GetDataLength() == 10);
  BOOST_TEST(dv4.GetMaxLength() == 10);
  BOOST_TEST(dv4.GetPersistenceLength() == 11);
  BOOST_TEST(dv1 < dv4);
  BOOST_TEST(dv1 <= dv4);
  BOOST_TEST(dv1 != dv4);
  BOOST_TEST(std::any_cast<string>(dv4.GetValue()) == "abcd     ");

  dv2 = dv4;
  BOOST_TEST(dv4 == dv2);

  Byte buf[2000] = "abcd     ";
  DataValueFixChar dv6(buf, 10, true);
  BOOST_TEST(std::any_cast<string>(dv6.GetValue()) == "abcd     ");
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

  DataValueFixChar dv9(pStr, strlen(pStr));
  dv9.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv9);

  DataValueFixChar dv10(pStr, strlen(pStr), 10, true);
  dv10.WriteData(buf + 30);
  dv3.ReadData(buf + 30);
  BOOST_TEST(dv3 == dv10);

  DataValueFixChar *pDv = dv10.Clone();
  BOOST_TEST(pDv->GetValueType() == ValueType::NULL_VALUE);
  delete pDv;

  pDv = dv10.Clone(true);
  BOOST_TEST(pDv->GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST((string)(*pDv) == "abcd     ");
  delete pDv;

  StrBuff sb(0);
  dv1 = "abcdefghijklmn1234567890";
  dv1.ToString(sb);
  BOOST_TEST(strncmp(sb.GetBuff(), "abcdefghijklmn1234567890", 24) == 0);
  BOOST_TEST(sb.GetBufLen() > 999U);
  BOOST_TEST(sb.GetStrLen() == 999U);
}

BOOST_AUTO_TEST_CASE(DataValueFixCharCopy_test) {
  class DataValueFixCharEx : public DataValueFixChar {
  public:
    using DataValueFixChar::bysValue_;
    using DataValueFixChar::DataValueFixChar;
    using DataValueFixChar::maxLength_;
  };

  DataValueInt dvi(100, true);
  DataValueFixCharEx dvc(10);

  dvc.Copy(dvi);
  BOOST_TEST(strcmp((char *)dvc.bysValue_, "100      ") == 0);
  BOOST_TEST(dvc.GetValueType() == ValueType::SOLE_VALUE);

  const char *pStr = "abcdefghijklmn";
  DataValueFixCharEx dvc2(pStr, 14);

  try {
    dvc.Copy(dvc2);
  } catch (utils::ErrorMsg &err) {
    BOOST_TEST(err.getErrId() == DT_INPUT_OVER_LENGTH);
  }

  char buf[100];
  buf[0] = VALUE_TYPE | ((Byte)DataType::FIXCHAR & DATE_TYPE);
  strcpy(buf + 1, "abcdefg  ");

  dvc2 = DataValueFixCharEx(10);
  dvc2.ReadData((Byte *)buf, 0, false);
  dvc.Copy(dvc2);
  BOOST_TEST(dvc.bysValue_ == dvc2.bysValue_);
  BOOST_TEST(dvc.maxLength_ == dvc2.maxLength_);
  BOOST_TEST(dvc == dvc2);

  dvc2.ReadData((Byte *)buf, 0, true);
  dvc.Copy(dvc2, false);
  BOOST_TEST(dvc.bysValue_ != dvc2.bysValue_);
  BOOST_TEST(dvc == dvc2);

  dvc.Copy(dvc2, true);
  BOOST_TEST(dvc2.GetValueType() == ValueType::NULL_VALUE);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
