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
  BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetPersistenceLength() == 1);
  BOOST_TEST(!dv1.GetValue().has_value());

  DataValueBlob dv2(100, false);
  BOOST_TEST(dv2.GetMaxLength() == 100);
  BOOST_TEST(dv2.GetDataLength() == 0);
  BOOST_TEST(dv2.GetPersistenceLength() == 1);

  const char *pStr = "abcd";
  DataValueBlob dv4(pStr, 4);
  BOOST_TEST(dv4.GetDataType() == DataType::BLOB);
  BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv4.IsNull());
  BOOST_TEST(dv4.GetDataLength() == 4);
  BOOST_TEST(dv4.GetMaxLength() == DEFAULT_MAX_LEN);
  BOOST_TEST(dv4.GetPersistenceLength() == 9);

  dv2 = dv4;
  BOOST_TEST(dv4 == dv2);

  DataValueBlob dv5(pStr, 4, 100, false);
  BOOST_TEST(dv5.GetDataLength() == 4);
  BOOST_TEST(dv5.GetMaxLength() == 100);
  BOOST_TEST(dv5.GetPersistenceLength() == 9);

  Byte buf[100];
  DataValueBlob dv9(pStr, 4);
  dv9.WriteData(buf + 20);
  dv1.ReadData(buf + 20);
  BOOST_TEST(dv1 == dv9);

  DataValueBlob dv10(pStr, 4, 100, false);
  dv10.WriteData(buf + 30);
  dv5.ReadData(buf + 30, dv10.GetDataLength());
  BOOST_TEST(dv5 == dv10);

  StrBuff sb(0);
  *(int *)buf = 6;
  for (int i = 4; i < 10; i++) {
    buf[i] = (char)i;
  }

  dv1 = (char *)buf;
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

  DataValueInt dvi(100, true);
  DataValueBlobEx dvb(10);

  try {
    dvb.Copy(dvi);
  } catch (ErrorMsg &err) {
    BOOST_TEST(err.getErrId() == DT_UNSUPPORT_CONVERT);
  }

  const char *pStr = "abcdefghijklmn";
  DataValueBlobEx dvb2(pStr, 14);

  try {
    dvb.Copy(dvb2);
  } catch (ErrorMsg &err) {
    BOOST_TEST(err.getErrId() == DT_INPUT_OVER_LENGTH);
  }

  char buf[100];
  buf[0] = VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE);
  *(int *)(buf + 1) = 7;
  memcpy(buf + 5, "abcdefg", 7);

  dvb2.ReadData((Byte *)buf, 0, false);
  dvb.Copy(dvb2);
  BOOST_TEST(dvb.bysValue_ == dvb2.bysValue_);
  BOOST_TEST(dvb.maxLength_ != dvb2.maxLength_);
  BOOST_TEST(dvb.soleLength_ == dvb2.soleLength_);
  BOOST_TEST(dvb == dvb2);

  dvb2.ReadData((Byte *)buf, 0, true);
  dvb.Copy(dvb2, false);
  BOOST_TEST(dvb.bysValue_ != dvb2.bysValue_);
  BOOST_TEST(dvb.maxLength_ != dvb2.maxLength_);
  BOOST_TEST(dvb.soleLength_ == dvb2.soleLength_);
  BOOST_TEST(dvb == dvb2);

  dvb.Copy(dvb2, true);
  BOOST_TEST(dvb2.GetValueType() == ValueType::NULL_VALUE);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
