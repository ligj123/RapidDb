#include <boost/test/unit_test.hpp>
#include <string>
#include "../../src/utils/ErrorMsg.h"

using namespace std;

namespace utils {
	BOOST_AUTO_TEST_SUITE(UtilsTest)

	BOOST_AUTO_TEST_CASE(ErrorMsg_test)
	{
		ErrorMsg err1(1);
		BOOST_TEST(err1.what(), "���Թ���");
	
		ErrorMsg err2(2, { "e1", "e2" });
		BOOST_TEST(err2.what(), "table error1-e1, error2-e2");
	}
	BOOST_AUTO_TEST_SUITE_END()
}