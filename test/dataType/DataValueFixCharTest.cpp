#include <boost/test/unit_test.hpp>
#include "../../src/dataType/DataValueFixChar.h"

namespace storage {
  BOOST_AUTO_TEST_SUITE(DataTypeTest)
BOOST_AUTO_TEST_CASE(DataValueFixChar_test)
{
		DataValueFixChar dv1;
		BOOST_TEST(dv1.GetDataType() == DataType::FIXCHAR);
		BOOST_TEST(dv1.GetValueType() == ValueType::NULL_VALUE);
		BOOST_TEST(dv1.IsFixLength());
		BOOST_TEST(dv1.IsNull());
		BOOST_TEST(dv1.GetMaxLength() == DEFAULT_MAX_LEN);
		BOOST_TEST(dv1.GetLength() == 0);
		BOOST_TEST(dv1.GetPersistenceLength() == 1);
		BOOST_TEST(!dv1.GetValue().has_value());

		DataValueFixChar dv2(100, true);
		BOOST_TEST(dv2.GetMaxLength() == 100);
		BOOST_TEST(dv2.GetLength() == 100);
		BOOST_TEST(dv2.GetPersistenceLength() == 100);
		DataValueFixChar dv3(100, true, utils::Charsets::GBK);
		BOOST_TEST(dv1 == dv2);
		BOOST_TEST(dv1 == dv3);

		DataValueFixChar dv4("abcd");
		BOOST_TEST(dv4.GetDataType() == DataType::FIXCHAR);
		BOOST_TEST(dv4.GetValueType() == ValueType::SOLE_VALUE);
		BOOST_TEST(!dv4.IsNull());
		BOOST_TEST(dv4.GetLength() == DEFAULT_MAX_LEN);
		BOOST_TEST(dv4.GetMaxLength() == DEFAULT_MAX_LEN);
		BOOST_TEST(dv4.GetPersistenceLength() == DEFAULT_MAX_LEN + 1);
		BOOST_TEST(dv1 < dv4);
		BOOST_TEST(dv1 <= dv4);
		BOOST_TEST(dv1 != dv4);
		BOOST_TEST(std::any_cast<string>(dv4.GetValue()) == "abcd");

		dv2 = dv4;
		BOOST_TEST(dv4 == dv2);

		DataValueFixChar dv5("abcd", 100, true, utils::Charsets::UTF8);
		BOOST_TEST(dv5.GetLength() == 100);
		BOOST_TEST(dv5.GetMaxLength() == 100);
		BOOST_TEST(dv5.GetPersistenceLength() == 100);

		char buf[100] ="abcd";
		DataValueFixChar dv6(100, buf, true, utils::Charsets::UTF8);
		BOOST_TEST(std::any_cast<string>(dv6.GetValue()) == "abcd");
		BOOST_TEST(dv6.GetValueType() == ValueType::BYTES_VALUE);

		string str = std::any_cast<string>(dv6.GetValue());

		BOOST_TEST(dv6 > dv1);
		BOOST_TEST(dv6 >= dv1);
		BOOST_TEST(dv6 != dv1);

		dv1.SetDefaultValue();
		BOOST_TEST((string)dv1 == "");

		dv1.SetMaxValue();
		BOOST_TEST((string)dv1 == "\\uff\\uff\\uff");

		dv1.SetMinValue();
		BOOST_TEST((string)dv1 == "");

		DataValueFixChar dv7;
		dv7.WriteData(buf);
		dv1.ReadData(buf, -1);
		BOOST_TEST(dv1 == dv7);

		DataValueFixChar dv9("abcd", 50);
		dv9.WriteData(buf + 20);
		dv1.ReadData(buf + 20);
		BOOST_TEST(dv1 == dv9);

		DataValueFixChar dv10("abcd", 50, true);
		dv10.WriteData(buf + 30);
		dv3.ReadData(buf + 30);
		BOOST_TEST(dv3 == dv10);
	}
	BOOST_AUTO_TEST_SUITE_END()
}