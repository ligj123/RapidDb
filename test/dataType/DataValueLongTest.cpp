#include <boost/test/unit_test.hpp>
#include "../../src/dataType/DataValueLong.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(DataTypeTest)

	BOOST_AUTO_TEST_CASE(DataValueLong_test)
	{
		DataValueLong dv1;
		DataValueLong dv4(dv1);
		DataValueLong dv5 = dv1;

		BOOST_TEST(dv1.GetDataType() == DataType::LONG);
		BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
		BOOST_TEST(dv1.IsFixLength());
		BOOST_TEST(dv1.IsNull());
		BOOST_TEST(dv1.GetMaxLength() == 8);
		BOOST_TEST(dv1.GetLength(true) == 8);
		BOOST_TEST(dv1.GetLength(false) == 1);
		BOOST_TEST(dv1 == dv5);
		BOOST_TEST(dv1 == dv4);
		BOOST_TEST(!dv1.GetValue().has_value());

		DataValueLong dv2(2);
		BOOST_TEST(dv2.GetDataType() == DataType::LONG);
		BOOST_TEST(dv2.GetValueType() == ValueType::SOLE_VALUE);
		BOOST_TEST(!dv2.IsNull());
		BOOST_TEST(dv2.GetLength(true) == 8);
		BOOST_TEST(dv2.GetLength(false) == 9);
		BOOST_TEST(dv1 < dv2);
		BOOST_TEST(dv1 <= dv2);
		BOOST_TEST(dv1 != dv2);
		BOOST_TEST(std::any_cast<int64_t>(dv2.GetValue()) == 2);

		dv4 = dv2;
		BOOST_TEST(dv4 == dv2);

		char buf[100];
		*((int64_t*)buf) = 10;
		DataValueLong dv3(buf);
		BOOST_TEST(std::any_cast<int64_t>(dv3.GetValue()) == 10);
		BOOST_TEST(dv3.GetValueType() == ValueType::BYTES_VALUE);

		BOOST_TEST(dv3 > dv2);
		BOOST_TEST(dv3 >= dv2);
		BOOST_TEST(dv3 != dv2);

		dv1.SetDefaultValue();
		BOOST_TEST((int64_t)dv1 == 0L);

		dv1.SetMaxValue();
		BOOST_TEST((int64_t)dv1 == LLONG_MAX);

		dv1.SetMinValue();
		BOOST_TEST((int64_t)dv1 == LLONG_MIN);

		DataValueLong dv7;
		dv7.WriteData(buf, true);
		dv1.ReadData(buf, true, -1);
		BOOST_TEST(dv1 == dv7);

		dv7.WriteData(buf + 10, false);
		dv1.ReadData(buf + 10, false);
		BOOST_TEST(dv1 == dv7);

		DataValueLong dv8(10000);
		dv8.WriteData(buf + 20, true);
		dv1.ReadData(buf + 20, true);
		BOOST_TEST(dv1 == dv8);

		dv8.WriteData(buf + 30, false);
		dv1.ReadData(buf + 30, false);
		BOOST_TEST(dv1 == dv8);
	}
	BOOST_AUTO_TEST_SUITE_END()
}