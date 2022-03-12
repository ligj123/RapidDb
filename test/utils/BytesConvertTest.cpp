#include "../../src/utils/BytesConvert.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(BytesConvert_test) {
  int64_t val = 0x0102030405060708LL;
  int64_t rt = Int64FromBytes((Byte *)&val, false);
  BOOST_TEST(rt == 0x0102030405060708LL);
  rt = Int64FromBytes((Byte *)&val, true);
  BOOST_TEST(rt == 0x8807060504030201LL);

  Int64ToBytes(0x8807060504030201LL, (Byte *)&rt, true);
  BOOST_TEST(rt == val);
  Int64ToBytes(0x0102030405060708LL, (Byte *)&rt, false);
  BOOST_TEST(rt == val);

  uint64_t uval = 0x0102030405060708LL;
  uint64_t urt = UInt64FromBytes((Byte *)&uval, false);
  BOOST_TEST(urt == 0x0102030405060708LL);
  urt = UInt64FromBytes((Byte *)&uval, true);
  BOOST_TEST(urt == 0x0807060504030201LL);

  UInt64ToBytes(0x0807060504030201LL, (Byte *)&urt, true);
  BOOST_TEST(urt == uval);
  UInt64ToBytes(0x0102030405060708LL, (Byte *)&urt, false);
  BOOST_TEST(urt == uval);

  int32_t ival = 0x01020304;
  int32_t irt = Int32FromBytes((Byte *)&ival, false);
  BOOST_TEST(irt == 0x01020304);
  irt = Int32FromBytes((Byte *)&ival, true);
  BOOST_TEST(irt == 0x84030201);

  Int32ToBytes(0x84030201, (Byte *)&irt, true);
  BOOST_TEST(irt == ival);
  Int32ToBytes(0x01020304, (Byte *)&irt, false);
  BOOST_TEST(irt == ival);

  uint32_t uival = 0x01020304;
  uint32_t uirt = UInt32FromBytes((Byte *)&uival, false);
  BOOST_TEST(uirt == 0x01020304);
  uirt = UInt32FromBytes((Byte *)&uival, true);
  BOOST_TEST(uirt == 0x04030201);

  UInt32ToBytes(0x04030201, (Byte *)&uirt, true);
  BOOST_TEST(uirt == uival);
  UInt32ToBytes(0x01020304, (Byte *)&uirt, false);
  BOOST_TEST(uirt == uival);

  int16_t sval = 0x0102;
  int16_t srt = Int16FromBytes((Byte *)&sval, false);
  BOOST_TEST(srt == 0x0102);
  srt = Int16FromBytes((Byte *)&sval, true);
  BOOST_TEST(srt == (short)0x8201);

  Int16ToBytes((short)0x8201, (Byte *)&srt, true);
  BOOST_TEST(srt == sval);
  Int16ToBytes(0x0102, (Byte *)&srt, false);
  BOOST_TEST(srt == sval);

  uint16_t usval = 0x0102;
  uint16_t usrt = UInt16FromBytes((Byte *)&usval, false);
  BOOST_TEST(usrt == 0x0102);
  usrt = UInt16FromBytes((Byte *)&usval, true);
  BOOST_TEST(usrt == 0x0201);

  UInt16ToBytes(0x0201, (Byte *)&usrt, true);
  BOOST_TEST(usrt == usval);
  UInt16ToBytes(0x0102, (Byte *)&usrt, false);
  BOOST_TEST(usrt == usval);

  double dval = -0.0123456;
  Byte bys[20];
  DoubleToBytes(dval, bys, true);
  double drt = DoubleFromBytes(bys, true);
  BOOST_TEST(dval == drt);

  DoubleToBytes(dval, bys, false);
  drt = DoubleFromBytes(bys, false);
  BOOST_TEST(dval == drt);

  dval = 123456.789;
  DoubleToBytes(dval, bys, true);
  drt = DoubleFromBytes(bys, true);
  BOOST_TEST(dval == drt);

  DoubleToBytes(dval, bys, false);
  drt = DoubleFromBytes(bys, false);
  BOOST_TEST(dval == drt);

  DoubleToBytes(-0.0123456, bys, true);
  DoubleToBytes(-0.0123457, bys + 8, true);
  BOOST_TEST(memcmp(bys, bys + 8, 8) > 0);

  DoubleToBytes(0.0123456, bys, true);
  DoubleToBytes(0.0123457, bys + 8, true);
  BOOST_TEST(memcmp(bys, bys + 8, 8) < 0);

  DoubleToBytes(-123456.0, bys, true);
  DoubleToBytes(-123457.0, bys + 8, true);
  BOOST_TEST(memcmp(bys, bys + 8, 8) > 0);

  DoubleToBytes(123456.0, bys, true);
  DoubleToBytes(123457.0, bys + 8, true);
  BOOST_TEST(memcmp(bys, bys + 8, 8) < 0);

  DoubleToBytes(123456.0, bys, true);
  DoubleToBytes(-123456.0, bys + 8, true);
  BOOST_TEST(memcmp(bys, bys + 8, 8) > 0);

  float fval = -0.0123456f;
  FloatToBytes(fval, bys, true);
  float frt = FloatFromBytes(bys, true);
  float gap = fval - frt;
  BOOST_TEST((gap < 0.00000001 && gap > -0.00000001));

  FloatToBytes(fval, bys, false);
  frt = FloatFromBytes(bys, false);
  gap = fval - frt;
  BOOST_TEST((gap < 0.00000001 && gap > -0.00000001));

  fval = 123456.7f;
  FloatToBytes(fval, bys, true);
  frt = FloatFromBytes(bys, true);
  gap = fval - frt;
  BOOST_TEST((gap < 0.00000001 && gap > -0.00000001));

  FloatToBytes(fval, bys, false);
  frt = FloatFromBytes(bys, false);
  gap = fval - frt;
  BOOST_TEST((gap < 0.00000001 && gap > -0.00000001));
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
