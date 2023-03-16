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
  BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_FIX_LEN);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength(SavePosition::KEY) ==
             DEFAULT_MAX_FIX_LEN);
  BOOST_TEST(dv1.GetPersistenceLength(SavePosition::VALUE) == 0);
  BOOST_TEST(!dv1.GetValue().has_value());

  const char *pStr = "abcd";
  DataValueFixChar dv2(pStr, 4);
  BOOST_TEST(dv2.GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(dv2.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv2.IsNull());
  BOOST_TEST(dv2.GetDataLength() == 5);
  BOOST_TEST(dv2.GetMaxLength() == 5);
  BOOST_TEST(dv2.GetPersistenceLength(SavePosition::KEY) == 5);
  BOOST_TEST(dv2.GetPersistenceLength(SavePosition::VALUE) == 5);
  BOOST_TEST(std::any_cast<MString>(dv2.GetValue()) == "abcd");
  BOOST_TEST((MString)dv2 == "abcd");
  BOOST_TEST((string)dv2 == "abcd");
  BOOST_TEST(dv1 < dv2);
  BOOST_TEST(dv1 <= dv2);
  BOOST_TEST(dv1 != dv2);

  DataValueFixChar dv3(pStr, 4, 10);
  BOOST_TEST(dv3.GetDataType() == DataType::FIXCHAR);
  BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv3.IsNull());
  BOOST_TEST(dv3.GetDataLength() == 10);
  BOOST_TEST(dv3.GetMaxLength() == 10);
  BOOST_TEST(dv3.GetPersistenceLength(SavePosition::KEY) == 10);
  BOOST_TEST(dv3.GetPersistenceLength(SavePosition::VALUE) == 10);
  BOOST_TEST(std::any_cast<MString>(dv3.GetValue()) == "abcd     ");
  BOOST_TEST((MString)dv3 == "abcd");
  BOOST_TEST((string)dv3 == "abcd");

  DataValueFixChar *dv4 = dv1.Clone(true);
  BOOST_TEST(dv4->IsNull());
  BOOST_TEST(dv4->GetDataLength() == 0);
  BOOST_TEST(dv4->GetMaxLength() == DEFAULT_MAX_FIX_LEN);

  Byte buf[DEFAULT_MAX_FIX_LEN + 100];
  uint32_t len = dv1.WriteData(buf, SavePosition::KEY);
  BOOST_TEST(len == DEFAULT_MAX_FIX_LEN);
  dv4->ReadData(buf, DEFAULT_MAX_FIX_LEN, SavePosition::KEY);
  BOOST_TEST(dv4->GetDataLength() == DEFAULT_MAX_FIX_LEN);
  len = dv1.WriteData(buf, SavePosition::VALUE);
  BOOST_TEST(len == 0);
  dv4->ReadData(buf, 0, SavePosition::VALUE);
  BOOST_TEST(dv4->IsNull());
  delete dv4;

  dv4 = dv2.Clone(false);
  BOOST_TEST(dv4->IsNull());
  BOOST_TEST(dv4->GetDataLength() == 0);
  BOOST_TEST(dv4->GetMaxLength() == 5);
  delete dv4;

  dv4 = dv2.Clone(true);
  BOOST_TEST(!dv4->IsNull());
  BOOST_TEST(dv4->GetDataLength() == 5);
  BOOST_TEST(dv4->GetMaxLength() == 5);
  BOOST_TEST((string)*dv4 == "abcd");

  len = dv2.WriteData(buf, SavePosition::KEY);
  BOOST_TEST(len == 5);
  dv4->ReadData(buf, 5, SavePosition::KEY);
  BOOST_TEST((string)*dv4 == "abcd");
  len = dv2.WriteData(buf, SavePosition::VALUE);
  BOOST_TEST(len == 5);
  dv4->ReadData(buf, 5, SavePosition::VALUE);
  BOOST_TEST((string)*dv4 == "abcd");

  BOOST_TEST(*dv4 == dv2);
  BOOST_TEST(dv4->EQ(dv2));
  delete dv4;

  dv3.SetValue("abcdefg");
  BOOST_TEST((string)dv3 == "abcdefg  ");
  BOOST_TEST(any_cast<MString>(dv3.GetValue()) == "abcdefg  ");
  dv3.SetNull();
  BOOST_TEST(dv3.IsNull());

  dv3.PutValue(123);
  BOOST_TEST((string)dv3 == "123      ");
  dv3.PutValue(string("abcd12345"));
  BOOST_TEST((string)dv3 == "abcd12345");

  dv3.Copy(dv2);
  BOOST_TEST((string)dv3 == "abcd     ");

  dv3.SetMinValue();
  BOOST_TEST((string)dv2 == "");
  dv3.SetMaxValue();
  Byte *p = dv3.GetBuff();
  BOOST_TEST((p[0] == 0xff && p[1] == 0xff && p[2] == 0xff));
  dv3.SetDefaultValue();
  BOOST_TEST((string)dv2 == "         ");

  dv3 = "abcd";
  BOOST_TEST((string)dv3 == "abcd     ");
  dv3 = MString("abcde");
  BOOST_TEST((string)dv3 == "abcde    ");
  dv3 = string("abcdef");
  BOOST_TEST((string)dv3 == "abcdef   ");

  BOOST_TEST(dv3.GT(dv2));
  BOOST_TEST(dv2.LT(dv3));

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

  DataValueInt dvi(100);
  DataValueFixCharEx dvc(10);

  dvc.Copy(dvi);
  BOOST_TEST(strcmp((char *)dvc.bysValue_, "100      ") == 0);
  BOOST_TEST(dvc.GetValueType() == ValueType::SOLE_VALUE);

  const char *pStr = "abcdefghijklmn";
  DataValueFixCharEx dvc2(pStr, 14);

  bool b = dvc.Copy(dvc2);
  BOOST_TEST(b);
  BOOST_TEST(_threadErrorMsg->getErrId() == DT_INPUT_OVER_LENGTH);

  char buf[100];
  strcpy(buf, "abcdefg  ");

  dvc2 = DataValueFixCharEx(10);
  dvc2.ReadData((Byte *)buf, 10, SavePosition::VALUE, false);
  dvc.Copy(dvc2);
  BOOST_TEST(dvc.bysValue_ == dvc2.bysValue_);
  BOOST_TEST(dvc.maxLength_ == dvc2.maxLength_);
  BOOST_TEST(dvc == dvc2);

  dvc2.ReadData((Byte *)buf, 10, SavePosition::VALUE, true);
  dvc.Copy(dvc2, false);
  BOOST_TEST(dvc.bysValue_ != dvc2.bysValue_);
  BOOST_TEST(dvc == dvc2);

  dvc.Copy(dvc2, true);
  BOOST_TEST(dvc2.GetValueType() == ValueType::NULL_VALUE);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
