#include "../../src/dataType/DataValueDigit.h"
#include "../../src/dataType/DataValueVarChar.h"
#include <boost/test/unit_test.hpp>

namespace storage {
BOOST_AUTO_TEST_SUITE(DataTypeTest)
BOOST_AUTO_TEST_CASE(DataValueDigital_test) {
  DataValueLong dv1;
  BOOST_TEST(dv1.GetDataType() == DataType::LONG);
  BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
  BOOST_TEST(dv1.IsFixLength());
  BOOST_TEST(dv1.IsNull());
  BOOST_TEST(dv1.GetMaxLength() == 8);
  BOOST_TEST(dv1.GetDataLength() == 0);
  BOOST_TEST(dv1.GetSavePosition() == SavePosition::ALL);
  BOOST_TEST(dv1.GetPersistenceLength(SavePosition::KEY) == 8);
  BOOST_TEST(dv1.GetPersistenceLength(SavePosition::VALUE) == 0);
  BOOST_TEST(!dv1.GetValue().has_value());
  BOOST_TEST((int64_t)dv1 == 0LL);

  DataValueLong dv2(100);
  BOOST_TEST(dv2.GetDataType() == DataType::LONG);
  BOOST_TEST(dv2.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(!dv2.IsNull());
  BOOST_TEST(dv2.GetDataLength() == 8);
  BOOST_TEST(dv2.GetMaxLength() == 8);
  BOOST_TEST(dv2.GetPersistenceLength(SavePosition::KEY) == 8);
  BOOST_TEST(dv2.GetPersistenceLength(SavePosition::KEY) == 8);
  BOOST_TEST((int64_t)dv2 == 100);
  dv2.SetValue(200);
  BOOST_TEST((int64_t)dv2 == 200);
  dv2.SetNull();
  BOOST_TEST(dv2.IsNull());
  dv2.PutValue(any(300));
  BOOST_TEST((int64_t)dv2 == 300);
  BOOST_TEST(dv2.GetValueType() == ValueType::SOLE_VALUE);
  BOOST_TEST(dv2.GetLong() == 300);
  BOOST_TEST(dv2.GetDouble() == 300);
  BOOST_TEST(std::any_cast<int64_t>(dv2.GetValue()) == 300);

  BOOST_TEST(dv1 < dv2);
  BOOST_TEST(dv1 <= dv2);
  BOOST_TEST(dv1 != dv2);

  DataValueLong *dv3 = dv2.Clone();
  BOOST_TEST(*dv3 == dv2);

  Byte buf[100];
  uint32_t len = dv1.WriteData(buf, SavePosition::KEY);
  BOOST_TEST(len == 8);
  BOOST_TEST(Int64FromBytes(buf, true) == 0);
  dv3->ReadData(buf, 8, SavePosition::KEY);
  BOOST_TEST(dv3->GetLong() == 0);
  len = dv1.WriteData(buf, SavePosition::VALUE);
  BOOST_TEST(len == 0);
  dv3->ReadData(buf, 0, SavePosition::VALUE);
  BOOST_TEST(dv3->IsNull());

  len = dv2.WriteData(buf, SavePosition::KEY);
  BOOST_TEST(len == 8);
  BOOST_TEST(Int64FromBytes(buf, true) == 300);
  dv3->ReadData(buf, 8, SavePosition::KEY);
  BOOST_TEST(dv3->GetLong() == 300);
  len = dv2.WriteData(buf, SavePosition::VALUE);
  BOOST_TEST(len == 8);
  BOOST_TEST(Int64FromBytes(buf, false) == 300);
  dv3->ReadData(buf, 8, SavePosition::VALUE);
  BOOST_TEST(dv3->GetLong() == 300);

  len = dv1.WriteData(buf);
  BOOST_TEST(len == 1);
  dv3->ReadData(buf);
  BOOST_TEST(dv3->IsNull());

  len = dv2.WriteData(buf);
  BOOST_TEST(len == 9);
  dv3->ReadData(buf);
  BOOST_TEST(dv3->GetLong() == 300);
  delete dv3;

  dv1 = 50;
  BOOST_TEST((int64_t)dv1 == 50);
  dv1 = dv2;
  BOOST_TEST((int64_t)dv1 == 300);
  BOOST_TEST((int64_t)dv2 == 300);

  dv2.Add(dv1);
  BOOST_TEST((int64_t)dv2 == 600);

  DataValueVarChar dvc("20", 2);
  dv2.Add(dvc);
  BOOST_TEST((int64_t)dv2 == 620);

  BOOST_TEST(!dv2.EQ(dv1));
  BOOST_TEST(dv2.GT(dv1));
  BOOST_TEST(!dv2.LT(dv1));

  dv1.SetDefaultValue();
  BOOST_TEST((int64_t)dv1 == 0L);

  dv1.SetMaxValue();
  BOOST_TEST((int64_t)dv1 == LLONG_MAX);

  dv1.SetMinValue();
  BOOST_TEST((int64_t)dv1 == LLONG_MIN);

  dv1.PutValue(any(15));
  BOOST_TEST((int64_t)dv1 == 15);
  dv1.PutValue(any(string("120")));
  BOOST_TEST((int64_t)dv1 == 120);

  StrBuff sb(0);
  dv1 = 1234567890;
  dv1.ToString(sb);
  BOOST_TEST(strcmp(sb.GetBuff(), "1234567890") == 0);
  BOOST_TEST(sb.GetBufLen() > 10U);
  BOOST_TEST(sb.GetStrLen() == 10U);

  dv1 = 100;
  DataValueUInt dvui(10);
  DataValueLong *pdv = dv1 + dvui;
  BOOST_TEST((void *)pdv == (void *)&dv1);
  BOOST_TEST(dv1.GetLong() == 110);

  pdv = dv1 - dvui;
  BOOST_TEST((void *)pdv == (void *)&dv1);
  BOOST_TEST(dv1.GetLong() == 100);

  pdv = dv1 * dvui;
  BOOST_TEST((void *)pdv == (void *)&dv1);
  BOOST_TEST(dv1.GetLong() == 1000);

  pdv = dv1 / dvui;
  BOOST_TEST((void *)pdv == (void *)&dv1);
  BOOST_TEST(dv1.GetLong() == 100);

  dvui = 0;
  _threadErrorMsg.reset(nullptr);
  pdv = dv1 / dvui;
  BOOST_TEST(pdv == nullptr);
  BOOST_TEST(dv1.GetLong() == 100);
  BOOST_TEST(_threadErrorMsg != nullptr);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
