#include <boost/test/unit_test.hpp>
#include "../../src/core/LeafRecord.h"
#include "../../src/utils/Utilitys.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/core/IndexTree.h"

namespace storage {

	BOOST_AUTO_TEST_SUITE(CoreTest)

	BOOST_AUTO_TEST_CASE(LeafRecord_test)
	{
		const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		DataValueLong* dvKey = new DataValueLong(100LL, true);
		DataValueLong* dvVal = new DataValueLong(200LL, false);
		vector<IDataValue*> vctKey = { dvKey->CloneDataValue() };
		vector<IDataValue*> vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
	}
	BOOST_AUTO_TEST_SUITE_END()
}