#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/core/IndexTree.h"
#include "../../src/utils/Utilitys.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/pool/PageDividePool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/utils/BytesConvert.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(CoreTest)

	BOOST_AUTO_TEST_CASE(IndexTreeInsertRecord_test)
	{
		const string FILE_NAME = "./dbTest/testIndexTreeInsertRecord" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = 50000;
		
		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		
		vctKey.push_back(dvKey->CloneDataValue());
		vctVal.push_back(dvVal->CloneDataValue());
		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100LL;
			LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal, indexTree->GetHeadPage()->ReadRecordStamp());
			indexTree->InsertRecord(rr);
		}

		indexTree->Close(true);
		
		indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		VectorLeafRecord vct;
		indexTree->QueryRecord(nullptr, nullptr, false, true, vct);
		for (int i = 0; i < ROW_COUNT; i++) {
			VectorDataValue v1, v2;
			vct[i]->GetListKey(v1);
			vct[i]->GetListValue(v2);
			BOOST_TEST(i == (int64_t)(*(DataValueLong*)v1[0]));
			BOOST_TEST((i + 100) == (int64_t)(*(DataValueLong*)v2[0]));
		}

		indexTree->Close(true);
		delete dvKey;
		delete dvVal;
		PageBufferPool::ClearPool();
		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToNonUniqueIndex_test) {
		const string FILE_NAME = "./dbTest/testIndexRepeatedKey" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = 30000;

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, true);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);

		vctKey.push_back(dvKey->CloneDataValue());
		vctVal.push_back(dvVal->CloneDataValue());
		Byte bys[100];

		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i % (ROW_COUNT / 3);
			utils::Int64ToBytes(i + 100, bys, true);
			LeafRecord* rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
			indexTree->InsertRecord(rr);
		}

		indexTree->Close(true);

		indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		VectorLeafRecord vct;
		indexTree->QueryRecord(nullptr, nullptr, false, true, vct);

		for (int i = 0; i < ROW_COUNT; i++) {
			VectorDataValue v1, v2;
			vct[i]->GetListKey(v1);
			vct[i]->GetListValue(v2);
			int64_t key = *(DataValueLong*)v1[0];
			int64_t val = *(DataValueLong*)v2[0];
			BOOST_TEST((val - 100) % (ROW_COUNT / 3) == key);
			BOOST_TEST(key == i / 3);
		}

		indexTree->Close(true);
		delete dvKey;
		delete dvVal;
		PageBufferPool::ClearPool();
		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToUniqueIndex_test) {
		const string FILE_NAME = "./dbTest/testIndexRepeatedKey" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = 30000;

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, true);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);

		vctKey.push_back(new DataValueLong(10, true));
		vctVal.push_back(new DataValueLong(100, true));

		LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal, 0);
		utils::ErrorMsg* err = indexTree->InsertRecord(rr);
		BOOST_TEST(err == nullptr);

		*((DataValueLong*)vctVal[0]) = 200;
		rr = new LeafRecord(indexTree, vctKey, vctVal, 1);
		err = indexTree->InsertRecord(rr);
		BOOST_TEST(err->getErrId() == CORE_REPEATED_RECORD);
	
		indexTree->Close(true);
		delete dvKey;
		delete dvVal;
		PageBufferPool::ClearPool();
		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_SUITE_END()
}