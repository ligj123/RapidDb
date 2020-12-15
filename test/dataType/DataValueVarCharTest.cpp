#include <boost/test/unit_test.hpp>
#include "../../src/dataType/DataValueVarChar.h"

namespace storage {
  BOOST_AUTO_TEST_SUITE(DataTypeTest)
BOOST_AUTO_TEST_CASE(DataValueVarChar_test)
{
		DataValueVarChar dv1;
		BOOST_TEST(dv1.GetDataType() == DataType::VARCHAR);
		BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
		BOOST_TEST(!dv1.IsFixLength());
		BOOST_TEST(dv1.IsNull());
		BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_LEN);
		BOOST_TEST(dv1.GetLength() == 0);
		BOOST_TEST(dv1.GetPersistenceLength() == 1);
		BOOST_TEST(!dv1.GetValue().has_value());

		DataValueVarChar dv2(100, true);
		BOOST_TEST(dv2.GetMaxLength() == 100);
		BOOST_TEST(dv2.GetLength() == 0);
		BOOST_TEST(dv2.GetPersistenceLength() == 0);
		DataValueVarChar dv3(100, true, utils::Charsets::GBK);
		BOOST_TEST(dv1 == dv2);
		BOOST_TEST(dv1 == dv3);

	  DataValueVarChar dv4("abcd");
		BOOST_TEST(dv4.GetDataType() == DataType::VARCHAR);
		BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
		BOOST_TEST(!dv4.IsNull());
		BOOST_TEST(dv4.GetLength() == 5);
		BOOST_TEST(dv4.GetMaxLength() == DEFAULT_MAX_LEN);
		BOOST_TEST(dv4.GetPersistenceLength() == 10);
		BOOST_TEST(dv1 < dv4);
		BOOST_TEST(dv1 <= dv4);
		BOOST_TEST(dv1 != dv4);
		BOOST_TEST(std::any_cast<string>(dv4.GetValue()) == "abcd");

		dv2 = dv4;
		BOOST_TEST(dv4 == dv2);

		DataValueVarChar dv5("abcd", 100, true, utils::Charsets::UTF8);
		BOOST_TEST(dv5.GetLength() == 5);
		BOOST_TEST(dv5.GetMaxLength() == 100);
		BOOST_TEST(dv5.GetPersistenceLength() == 5);

		char buf[100] ="abcd";
		DataValueVarChar dv6(buf, 4, 100, true, utils::Charsets::UTF8);
		BOOST_TEST(std::any_cast<string>(dv6.GetValue()) == "abcd");
		BOOST_TEST(dv6.GetValueType() == ValueType::BYTES_VALUE);

		BOOST_TEST(dv6 > dv1);
		BOOST_TEST(dv6 >= dv1);
		BOOST_TEST(dv6 != dv1);

		dv1.SetDefaultValue();
		BOOST_TEST((string)dv1 == "");

		dv1.SetMaxValue();
		BOOST_TEST((string)dv1 == "\\uff\\uff\\uff");

		dv1.SetMinValue();
		BOOST_TEST((string)dv1 == "");

		DataValueVarChar dv7;
		dv7.WriteData(buf);
		dv1.ReadData(buf, -1);
		BOOST_TEST(dv1 == dv7);

		DataValueVarChar dv8(true);
		dv8.WriteData(buf + 10);
		dv2.ReadData(buf + 10);
		BOOST_TEST(dv2 == dv8);

		DataValueVarChar dv9("abcd");
		dv9.WriteData(buf + 20);
		dv1.ReadData(buf + 20);
		BOOST_TEST(dv1 == dv9);

		DataValueVarChar dv10("abcd", true);
		dv10.WriteData(buf + 30);
		dv2.ReadData(buf + 30);
		BOOST_TEST(dv2 == dv10);
	}
	BOOST_AUTO_TEST_SUITE_END()
}