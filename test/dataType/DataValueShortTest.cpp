#include <boost/test/unit_test.hpp>
#include "../../src/dataType/DataValueShort.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(DataTypeTest)

	BOOST_AUTO_TEST_CASE(DataValueShort_test)
	{
		DataValueShort dv1;
	
		BOOST_TEST(dv1.GetDataType() == DataType::SHORT);
		BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
		BOOST_TEST(dv1.IsFixLength());
		BOOST_TEST(dv1.IsNull());
		BOOST_TEST(dv1.GetMaxLength() == 3);
		BOOST_TEST(dv1.GetLength() == 0);
		BOOST_TEST(dv1.GetPersistenceLength() == 1);
		BOOST_TEST(!dv1.GetValue().has_value());

		DataValueShort dv2(true);
		BOOST_TEST(dv2.GetMaxLength() == 2);
		BOOST_TEST(dv2.GetLength() == 2);
		BOOST_TEST(dv2.GetPersistenceLength() == 2);
		BOOST_TEST(dv1 == dv2);

		DataValueShort dv3(2, false);
		BOOST_TEST(dv3.GetDataType() == DataType::SHORT);
		BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
		BOOST_TEST(!dv3.IsNull());
		BOOST_TEST(dv3.GetLength() == 2);
		BOOST_TEST(dv3.GetMaxLength() == 3);
		BOOST_TEST(dv3.GetPersistenceLength() == 3);
		BOOST_TEST(dv1 < dv3);
		BOOST_TEST(dv1 <= dv3);
		BOOST_TEST(dv1 != dv3);
		BOOST_TEST(std::any_cast<int16_t>(dv3.GetValue()) == 2);

		DataValueShort dv4(2, true);
		BOOST_TEST(dv4.GetPersistenceLength() == 2);
		BOOST_TEST(dv4 == dv3);

		char buf[100];
		*((int64_t*)buf) = 10;
		DataValueShort dv5(buf, false);
		BOOST_TEST(std::any_cast<int16_t>(dv5.GetValue()) == 10);
		BOOST_TEST(dv5.GetValueType() == ValueType::BYTES_VALUE);

		BOOST_TEST(dv5 > dv3);
		BOOST_TEST(dv5 >= dv3);
		BOOST_TEST(dv5 != dv3);
		
		char buf2[100];
		uint32_t len = dv5.WriteData(buf2);
		BOOST_TEST(len == 3);
		BOOST_TEST(buf2[0] == 1);
		BOOST_TEST(*((int16_t*)(buf2 + 1)) == 10);

		dv1.SetDefaultValue();
		BOOST_TEST((int16_t)dv1 == 0L);

		dv1.SetMaxValue();
		BOOST_TEST((int16_t)dv1 == SHRT_MAX);

		dv1.SetMinValue();
		BOOST_TEST((int16_t)dv1 == SHRT_MIN);

		DataValueShort dv6(true);
		dv6.WriteData(buf);
		dv2.ReadData(buf, -1);
		BOOST_TEST(dv2 == dv6);

		dv6 = 0x1234;
		dv6.WriteData(buf);
		dv2.ReadData(buf);
		BOOST_TEST(std::any_cast<int16_t>(dv2.GetValue()) == 0x1234);

		DataValueShort dv7(false);
		dv7.WriteData(buf + 20);
		dv1.ReadData(buf + 20);
		BOOST_TEST(dv1 == dv7);

		dv7 = 10000;
		dv7.WriteData(buf + 20);
		dv1.ReadData(buf + 20);
		BOOST_TEST(dv1 == dv7);

		DataValueShort dv8(dv7);
		BOOST_TEST(dv8 == dv7);

		dv6 = dv7;
		BOOST_TEST(dv6 == dv7);

		DataValueShort dv9(std::any(100));
		BOOST_TEST((int16_t)dv9 == 100);
	}
	BOOST_AUTO_TEST_SUITE_END()
}