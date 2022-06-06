#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/dataType/DataValueDigit.h"
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
  BOOST_TEST(dv1.GetPersistenceLength() == 0);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueVarChar dv2(100, true);
  BOOST_TEST(dv2.GetMaxLength() == 100);
  BOOST_TEST(dv2.GetDataLength() == 1);
  BOOST_TEST(dv2.GetPersistenceLength() == 1);
  BOOST_TEST(dv1 == dv2);

  const char *pStr = "abcd";
  DataValueVarChar dv4(pStr, (uint32_t)strlen(pStr));
  BOOST_TEST(dv4.GetDataType() == DataType::VARCHAR);
  BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv4.IsNull());
  BOOST_TEST(dv4.GetDataLength() == 5);
  BOOST_TEST(dv4.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv4.GetPersistenceLength() == 10);
  BOOST_TEST(dv1 < dv4);
  BOOST_TEST(dv1 <= dv4);
  BOOST_TEST(dv1 != dv4);
  BOOST_TEST(std::any_cast<MString>(dv4.GetValue()) == "abcd");

  dv2 = dv4;
  BOOST_TEST(dv4 == dv2);

  DataValueVarChar dv5(pStr, 4, 100, true);
  BOOST_TEST(dv5.GetDataLength() == 5);
  BOOST_TEST(dv5.GetMaxLength() == 100);
  BOOST_TEST(dv5.GetPersistenceLength() == 5);

  Byte buf[100] = "abcd";
  DataValueVarChar dv6((char *)buf, 4, 100, true);
  BOOST_TEST(std::any_cast<MString>(dv6.GetValue()) == "abcd");
  BOOST_TEST(dv6.GetValueType() == ValueType::SOLE_VALUE);

  BOOST_TEST(dv6 > dv1);
  BOOST_TEST(dv6 >= dv1);
  BOOST_TEST(dv6 != dv1);

  dv1.SetDefaultValue();
  BOOST_TEST((MString)dv1 == "");

  dv1.SetMaxValue();
  // BOOST_TEST((string)dv1 == "\\uff\\uff\\uff");

  dv1.SetMinValue();
  BOOST_TEST((MString)dv1 == "");

  DataValueVarChar dv7;
  dv7.WriteData(buf);
  dv1.ReadData(buf, 0);
  BOOST_TEST(dv1 == dv7);

  DataValueVarChar dv8(100, true);
  dv8.WriteData(buf + 10);
  dv2.ReadData(buf + 10, 1);
  BOOST_TEST(dv2.GetDataLength() == 1);
  BOOST_TEST(dv2.GetValueType() == ValueType::SOLE_VALUE);

  DataValueVarChar dv9(pStr, (uint32_t)strlen(pStr));
  dv9.WriteData(buf + 20);
  dv1.ReadData(buf + 20, 5);
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

BOOST_AUTO_TEST_CASE(DataValueVarCharCopy_test) {
  class DataValueVarCharEx : public DataValueVarChar {
  public:
    using DataValueVarChar::bysValue_;
    using DataValueVarChar::DataValueVarChar;
    using DataValueVarChar::maxLength_;
    using DataValueVarChar::soleLength_;
  };

  DataValueInt dvi(100, true);
  DataValueVarCharEx dvc(10);

  dvc.Copy(dvi);
  BOOST_TEST(strcmp((char *)dvc.bysValue_, "100") == 0);
  BOOST_TEST(dvc.soleLength_ == 4);
  BOOST_TEST(dvc.GetValueType() == ValueType::SOLE_VALUE);

  const char *pStr = "abcdefghijklmn";
  DataValueVarCharEx dvc2(pStr, 14);

  try {
    dvc.Copy(dvc2);
  } catch (ErrorMsg &err) {
    BOOST_TEST(err.getErrId() == DT_INPUT_OVER_LENGTH);
  }

  char buf[100];
  strcpy(buf, "abcdefg");

  dvc2.ReadData((Byte *)buf, 8, false);
  dvc.Copy(dvc2);
  BOOST_TEST(dvc.bysValue_ == dvc2.bysValue_);
  BOOST_TEST(dvc.maxLength_ != dvc2.maxLength_);
  BOOST_TEST(dvc.soleLength_ == dvc2.soleLength_);
  BOOST_TEST(dvc == dvc2);

  dvc2.ReadData((Byte *)buf, 8, true);
  dvc.Copy(dvc2, false);
  BOOST_TEST(dvc.bysValue_ != dvc2.bysValue_);
  BOOST_TEST(dvc.maxLength_ != dvc2.maxLength_);
  BOOST_TEST(dvc.soleLength_ == dvc2.soleLength_);
  BOOST_TEST(dvc == dvc2);

  dvc.Copy(dvc2, true);
  BOOST_TEST(dvc2.GetValueType() == ValueType::NULL_VALUE);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
