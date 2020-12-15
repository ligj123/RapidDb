#include <boost/test/unit_test.hpp>
#include "../../src/dataType/DataValueInt.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(DataTypeTest)

	BOOST_AUTO_TEST_CASE(DataValueInt_test)
	{
		DataValueInt dv1;
	
		BOOST_TEST(dv1.GetDataType() == DataType::INT);
		BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
		BOOST_TEST(dv1.IsFixLength());
		BOOST_TEST(dv1.IsNull());
		BOOST_TEST(dv1.GetMaxLength() == 5);
		BOOST_TEST(dv1.GetLength() == 0);
		BOOST_TEST(dv1.GetPersistenceLength() == 1);
		BOOST_TEST(!dv1.GetValue().has_value());

		DataValueInt dv2(true);
		BOOST_TEST(dv2.GetMaxLength() == 4);
		BOOST_TEST(dv2.GetLength() == 4);
		BOOST_TEST(dv2.GetPersistenceLength() == 4);
		BOOST_TEST(dv1 == dv2);

		DataValueInt dv3(2, false);
		BOOST_TEST(dv3.GetDataType() == DataType::INT);
		BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
		BOOST_TEST(!dv3.IsNull());
		BOOST_TEST(dv3.GetLength() == 4);
		BOOST_TEST(dv3.GetMaxLength() == 5);
		BOOST_TEST(dv3.GetPersistenceLength() == 5);
		BOOST_TEST(dv1 < dv3);
		BOOST_TEST(dv1 <= dv3);
		BOOST_TEST(dv1 != dv3);
		BOOST_TEST(std::any_cast<int32_t>(dv3.GetValue()) == 2);

		DataValueInt dv4(2, true);
		BOOST_TEST(dv4.GetPersistenceLength() == 4);
		BOOST_TEST(dv4 == dv3);

		char buf[100];
		*((int32_t*)buf) = 10;
		DataValueInt dv5(buf, false);
		BOOST_TEST(std::any_cast<int32_t>(dv5.GetValue()) == 10);
		BOOST_TEST(dv5.GetValueType() == ValueType::BYTES_VALUE);

		BOOST_TEST(dv5 > dv3);
		BOOST_TEST(dv5 >= dv3);
		BOOST_TEST(dv5 != dv3);
		
		char buf2[100];
		uint32_t len = dv5.WriteData(buf2);
		BOOST_TEST(len == 5);
		BOOST_TEST(buf2[0] == 1);
		BOOST_TEST(*((int32_t*)(buf2 + 1)) == 10);

		dv1.SetDefaultValue();
		BOOST_TEST((int32_t)dv1 == 0L);

		dv1.SetMaxValue();
		BOOST_TEST((int32_t)dv1 == INT_MAX);

		dv1.SetMinValue();
		BOOST_TEST((int32_t)dv1 == INT_MIN);

		DataValueInt dv6(true);
		dv6.WriteData(buf);
		dv2.ReadData(buf, -1);
		BOOST_TEST(dv2 == dv6);

		dv6 = 0x1234;
		dv6.WriteData(buf);
		dv2.ReadData(buf);
		BOOST_TEST(std::any_cast<int32_t>(dv2.GetValue()) == 0x1234);

		DataValueInt dv7(false);
		dv7.WriteData(buf + 20);
		dv1.ReadData(buf + 20);
		BOOST_TEST(dv1 == dv7);

		dv7 = 10000;
		dv7.WriteData(buf + 20);
		dv1.ReadData(buf + 20);
		BOOST_TEST(dv1 == dv7);

		DataValueInt dv8(dv7);
		BOOST_TEST(dv8 == dv7);

		dv6 = dv7;
		BOOST_TEST(dv6 == dv7);

		DataValueInt dv9(std::any(100));
		BOOST_TEST((int64_t)dv9 == 100);
	}
	BOOST_AUTO_TEST_SUITE_END()
}