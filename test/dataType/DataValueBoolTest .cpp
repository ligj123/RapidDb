#include <boost/test/unit_test.hpp>
#include "../../src/dataType/DataValueBool.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(DataTypeTest)

	BOOST_AUTO_TEST_CASE(DataValueBool_test)
	{
		DataValueBool dv1;
	
		BOOST_TEST(dv1.GetDataType() == DataType::BOOL);
		BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
		BOOST_TEST(dv1.IsFixLength());
		BOOST_TEST(dv1.IsNull());
		BOOST_TEST(dv1.GetMaxLength() == 1);
		BOOST_TEST(dv1.GetDataLength() == 0);
		BOOST_TEST(dv1.GetPersistenceLength() == 1);
		BOOST_TEST(!dv1.GetValue().has_value());

		DataValueBool dv2(true);
		BOOST_TEST(dv2.GetMaxLength() == 1);
		BOOST_TEST(dv2.GetDataLength() == 1);
		BOOST_TEST(dv2.GetPersistenceLength() == 1);
		BOOST_TEST(dv1 == dv2);

		DataValueBool dv3(true, false);
		BOOST_TEST(dv3.GetDataType() == DataType::BOOL);
		BOOST_TEST(dv3.GetValueType() == ValueType::SOLE_VALUE);
		BOOST_TEST(!dv3.IsNull());
		BOOST_TEST(dv3.GetDataLength() == 1);
		BOOST_TEST(dv3.GetMaxLength() == 1);
		BOOST_TEST(dv3.GetPersistenceLength() == 2);
		BOOST_TEST(dv1 < dv3);
		BOOST_TEST(dv1 <= dv3);
		BOOST_TEST(dv1 != dv3);
		BOOST_TEST(std::any_cast<bool>(dv3.GetValue()) == true);

		DataValueBool dv4(true, true);
		BOOST_TEST(dv4.GetPersistenceLength() == 1);
		BOOST_TEST(dv4 == dv3);

		Byte buf[100];
		*((uint8_t*)buf) = 1;
		DataValueBool dv5(buf, false);
		BOOST_TEST(std::any_cast<bool>(dv5.GetValue()) == true);
		BOOST_TEST(dv5.GetValueType() == ValueType::BYTES_VALUE);

		
		Byte buf2[100];
		uint32_t len = dv5.WriteData(buf2);
		BOOST_TEST(len == 2);
		BOOST_TEST(buf2[0] == 1);
		BOOST_TEST(*((uint8_t*)(buf2 + 1)) == 1);

		dv1.SetDefaultValue();
		BOOST_TEST((bool)dv1 == false);

		dv1.SetMaxValue();
		BOOST_TEST((bool)dv1 == true);

		dv1.SetMinValue();
		BOOST_TEST((bool)dv1 == false);

		DataValueBool dv6(true, true);
		dv6.WriteData(buf);
		dv2.ReadData(buf, -1);
		BOOST_TEST(dv2 == dv6);
	}
	BOOST_AUTO_TEST_SUITE_END()
}