#include "../../src/dataType/DataValueBlob.h"
#include "../../src/dataType/DataValueDigit.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)

BOOST_AUTO_TEST_CASE(DataValueBlob_test) {
  DataValueBlob dv1;
  BOOST_TEST(dv1.GetDataType() == DataType::BLOB);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(!dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_VAR_LEN);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength(SavePosition::VALUE) == 0);

  const char *pStr = "abcd";
  DataValueBlob dv2(pStr, (uint32_t)strlen(pStr));
  BOOST_TEST(dv2.GetDataType() == DataType::BLOB);
  BOOST_TEST(dv2.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv2.IsNull());
  BOOST_TEST(dv2.GetDataLength() == 4);
  BOOST_TEST(dv2.GetMaxLength() == 4);
  BOOST_TEST(dv2.GetPersistenceLength(SavePosition::VALUE) == 4);

  DataValueBlob dv3(pStr, 4, 10);
  BOOST_TEST(dv3.GetDataType() == DataType::BLOB);
  BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv3.IsNull());
  BOOST_TEST(dv3.GetDataLength() == 4);
  BOOST_TEST(dv3.GetMaxLength() == 10);
  BOOST_TEST(dv3.GetPersistenceLength(SavePosition::VALUE) == 4);

  DataValueBlob *dv4 = dv1.Clone(true);
  BOOST_TEST(dv4->IsNull());
  BOOST_TEST(dv4->GetDataLength() == 0);
  BOOST_TEST(dv4->GetMaxLength() == DEFAULT_MAX_VAR_LEN);

  Byte buf[1024];
  uint32_t len = dv1.WriteData(buf, SavePosition::VALUE);
  BOOST_TEST(len == 0);
  dv4->ReadData(buf, 0, SavePosition::VALUE);
  BOOST_TEST(dv4->IsNull());
  delete dv4;

  dv4 = dv2.Clone(false);
  BOOST_TEST(dv4->IsNull());
  BOOST_TEST(dv4->GetDataLength() == 0);
  BOOST_TEST(dv4->GetMaxLength() == 4);
  delete dv4;

  dv4 = dv2.Clone(true);
  BOOST_TEST(!dv4->IsNull());
  BOOST_TEST(dv4->GetDataLength() == 4);
  BOOST_TEST(dv4->GetMaxLength() == 4);

  len = dv2.WriteData(buf, SavePosition::VALUE);
  BOOST_TEST(len == 4);
  dv4->ReadData(buf, 4, SavePosition::VALUE);
  delete dv4;

  dv3.SetValue("abcdefg", 7);
  BOOST_TEST(memcmp(dv3.GetBuff(), "abcdefg", 7) == 0);
  dv3.SetNull();
  BOOST_TEST(dv3.IsNull());

  dv3.PutValue(123);
  BOOST_TEST(memcmp(dv3.GetBuff(), "123", 3) == 0);
  dv3.PutValue(string("abcd12345"));
  BOOST_TEST(memcmp(dv3.GetBuff(), "abcd12345", 9) == 0);

  dv3.Copy(dv2);
  BOOST_TEST(memcmp(dv3.GetBuff(), "abcd", 4) == 0);

  dv3.SetMinValue();
  BOOST_TEST(dv3.GetBuff()[0] == 0);
  dv3.SetMaxValue();
  Byte *p = dv3.GetBuff();
  BOOST_TEST((p[0] == 0xff && p[1] == 0xff && p[2] == 0xff));
  dv3.SetDefaultValue();
  BOOST_TEST((string)dv3 == "");

  StrBuff sb(0);
  for (int i = 0; i < 6; i++) {
    buf[i] = (char)(i + 4);
  }

  dv1.SetValue((char *)buf, 6);
  dv1.ToString(sb);
  BOOST_TEST(strcmp(sb.GetBuff(), "0x040506070809") == 0);
  BOOST_TEST(sb.GetBufLen() > 14U);
  BOOST_TEST(sb.GetStrLen() == 14U);
}

BOOST_AUTO_TEST_CASE(DataValueBlobCopy_test) {
  class DataValueBlobEx : public DataValueBlob {
  public:
    using DataValueBlob::bysValue_;
    using DataValueBlob::DataValueBlob;
    using DataValueBlob::maxLength_;
    using DataValueBlob::soleLength_;
  };

  DataValueInt dvi(100);
  DataValueBlobEx dvb(10);

  bool b = dvb.Copy(dvi);
  BOOST_TEST(!b);
  BOOST_TEST(_threadErrorMsg->getErrId() == DT_UNSUPPORT_CONVERT);

  const char *pStr = "abcdefghijklmn";
  DataValueBlobEx dvb2(pStr, 14);

  b = dvb.Copy(dvb2);
  BOOST_TEST(!b);
  BOOST_TEST(_threadErrorMsg->getErrId() == DT_INPUT_OVER_LENGTH);

  char buf[100];
  BytesCopy(buf, "abcdefg", 7);

  dvb2.ReadData((Byte *)buf, 7, SavePosition::VALUE, false);
  dvb.Copy(dvb2, true);
  BOOST_TEST(dvb.bysValue_ == (Byte *)buf);
  BOOST_TEST(dvb2.bysValue_ == nullptr);
  BOOST_TEST(dvb2.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(dvb != dvb2);

  dvb2.ReadData((Byte *)buf, 7, SavePosition::VALUE, false);
  dvb.Copy(dvb2, false);
  BOOST_TEST(dvb.bysValue_ == dvb2.bysValue_);
  BOOST_TEST(dvb.soleLength_ == dvb2.soleLength_);
  BOOST_TEST(dvb == dvb2);

  dvb2.ReadData((Byte *)buf, 7, SavePosition::VALUE, true);
  dvb.Copy(dvb2, false);
  BOOST_TEST(dvb.bysValue_ != dvb2.bysValue_);
  BOOST_TEST(dvb.soleLength_ == dvb2.soleLength_);
  BOOST_TEST(dvb == dvb2);

  dvb.Copy(dvb2, true);
  BOOST_TEST(dvb2.GetValueType() == ValueType::NULL_VALUE);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
