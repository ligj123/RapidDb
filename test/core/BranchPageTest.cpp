#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/core/BranchRecord.h"
#include "../../src/utils/Utilitys.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/core/IndexTree.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/core/BranchPage.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(CoreTest)

		BOOST_AUTO_TEST_CASE(BranchPage_test)
	{
		const string FILE_NAME = "./dbTest/testBranchPage" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = 100;

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		BranchPage* bp = (BranchPage*)indexTree->AllocateNewPage(UINT64_MAX, (Byte)1);

		vctKey.push_back(new DataValueLong(1LL, true));
		vctVal.push_back(new DataValueLong(1LL, true));
		RawKey key(vctKey);
		BOOST_TEST(!bp->RecordExist(key));

		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			BranchRecord* rr = new BranchRecord(indexTree, lr, i + 100);
			bp->InsertRecord(rr);
			lr->ReleaseRecord();
		}

		bp->SaveRecord();

		*((DataValueLong*)vctKey[0]) = 0;
		*((DataValueLong*)vctVal[0]) = 100;
		LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
		BranchRecord* rr = new BranchRecord(indexTree, lr, 100);
		BranchRecord* first = bp->GetRecordByPos(0);
		BOOST_TEST(rr->CompareTo(*first) == 0);
		lr->ReleaseRecord();
		rr->ReleaseRecord();
		first->ReleaseRecord();

		*((DataValueLong*)vctKey[0]) = ROW_COUNT - 1;
		*((DataValueLong*)vctVal[0]) = ROW_COUNT + 99;
		lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
		rr = new BranchRecord(indexTree, lr, ROW_COUNT + 99);
		BranchRecord* last = bp->GetRecordByPos(ROW_COUNT - 1);
		BOOST_TEST(rr->CompareTo(*last) == 0);
		lr->ReleaseRecord();
		rr->ReleaseRecord();
		last->ReleaseRecord();

		*((DataValueLong*)vctKey[0]) = ROW_COUNT / 2;
		*((DataValueLong*)vctVal[0]) = ROW_COUNT / 2 + 100;
		lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
		rr = new BranchRecord(indexTree, lr, ROW_COUNT / 2 + 100);
		BranchRecord* mid = bp->GetRecordByPos(ROW_COUNT / 2);
		BOOST_TEST(rr->CompareTo(*mid) == 0);
		lr->ReleaseRecord();
		rr->ReleaseRecord();
		mid->ReleaseRecord();

		indexTree->Close(true);
		delete dvKey;
		delete dvVal;

		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(BranchPageSave_test)
	{
		const string FILE_NAME = "./dbTest/testBranchPageSave" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = BranchPage::MAX_DATA_LENGTH / 22;
		
		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		BranchPage* bp = (BranchPage*)indexTree->AllocateNewPage(UINT64_MAX, (Byte)1);

		vctKey.push_back(dvKey->CloneDataValue(true));
		vctVal.push_back(dvVal->CloneDataValue(true));
		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			BranchRecord* rr = new BranchRecord(indexTree, lr, i);
			bp->InsertRecord(rr);
			lr->ReleaseRecord();
		}

		bp->SaveRecord();

		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			BranchRecord* rr = new BranchRecord(indexTree, lr, i);
			bool bFind;
			uint32_t index = bp->SearchRecord(*rr, bFind);
			BranchRecord* br = bp->GetRecordByPos(index);
			BOOST_TEST(br->CompareTo(*rr) == 0);

			lr->ReleaseRecord();
			br->ReleaseRecord();
			rr->ReleaseRecord();
		}

		indexTree->Close(true);
		delete dvKey;
		delete dvVal;

		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(BranchPageDelete_test)
	{
		const string FILE_NAME = "./dbTest/testBranchPage" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = 100;

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, true);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		BranchPage* bp = (BranchPage*)indexTree->AllocateNewPage(UINT64_MAX, (Byte)1);

		vctKey.push_back(dvKey->CloneDataValue(true));
		vctVal.push_back(dvVal->CloneDataValue(true));
		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			BranchRecord* rr = new BranchRecord(indexTree, lr, i + 100);
			bp->InsertRecord(rr);
			lr->ReleaseRecord();
		}

		bp->SaveRecord();
		for (int i = ROW_COUNT - 1; i >= 0; i--) {
			if (i % 2 == 1) {
				bp->DeleteRecord(i);
			}
		}

		bp->SaveRecord();
		BOOST_TEST(ROW_COUNT / 2 == bp->GetRecordSize());

		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			RawKey key(vctKey);

			BOOST_TEST((i % 2 != 1) == bp->RecordExist(key));
		}

		indexTree->Close(true);
		delete dvKey;
		delete dvVal;

		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(BranchPageSearchKey_test)
	{
		const string FILE_NAME = "./dbTest/testBranchPageSearchKey" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";

		DataValueVarChar* dvKey = new DataValueVarChar(1000, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		indexTree->GetHeadPage()->WriteKeyVariableFieldCount((short)1);
		BranchPage* bp = (BranchPage*)indexTree->AllocateNewPage(UINT64_MAX, (Byte)1);

		vctKey.push_back(dvKey->CloneDataValue(true));
		vctVal.push_back(dvVal->CloneDataValue(true));
		uint64_t arLong[] = { 1, 123456, 3456, 789, 8776 };
		for (int i = 0; i < 5; i++) {
			string str = "testString" + to_string(arLong[i]);
			*((DataValueVarChar*)vctKey[0]) = str.c_str();
			*((DataValueLong*)vctVal[0]) = i + 100;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			BranchRecord* rr = new BranchRecord(indexTree, lr, i + 100);
			bp->InsertRecord(rr);
			lr->ReleaseRecord();
		}

		uint64_t arKey[] = { 0, 20, 135, 70, 999 };
		int arPos[] = { 0, 2, 2, 3, 5 };
		for (int i = 0; i < 5; i++) {
			string str = "testString" + to_string(arKey[i]);
			*((DataValueVarChar*)vctKey[0]) = str.c_str();
			RawKey key(vctKey);
			bool bFind;
			BOOST_TEST(arPos[i] == bp->SearchKey(key, bFind));
		}

		bp->DecRefCount();
		indexTree->Close(true);
		delete dvKey;
		delete dvVal;

		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}
	BOOST_AUTO_TEST_SUITE_END()
}