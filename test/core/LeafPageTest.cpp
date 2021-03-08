#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/utils/Utilitys.h"
#include "../../src/core/LeafPage.h"
#include "../../src/core/LeafRecord.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/core/IndexTree.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/pool/PageDividePool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/pool/PageBufferPool.h"

namespace storage {
	BOOST_AUTO_TEST_SUITE(CoreTest)

	BOOST_AUTO_TEST_CASE(LeafPage_test)
	{
		const string FILE_NAME = "./dbTest/testLeafPage" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = 100;

		StoragePool::SetWriteSuspend(true);
		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		LeafPage* lp = (LeafPage*)indexTree->AllocateNewPage(UINT64_MAX, (Byte)0);

		BOOST_TEST(nullptr == lp->GetLastRecord());

		vctKey.push_back(dvKey->CloneDataValue());
		vctVal.push_back(dvVal->CloneDataValue());
		RawKey* key = new RawKey(vctKey);
		BOOST_TEST(nullptr == lp->GetRecord(*key));
		VectorLeafRecord vctLr;
		lp->GetRecords(*key, vctLr);
		BOOST_TEST(0 == vctLr.size());
		delete key;
		lp->WriteLock();
		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100LL;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			lp->InsertRecord(lr, (i % 2 == 0 ? -1 : i));
		}
		lp->WriteUnlock();
		lp->SaveRecords();

		*((DataValueLong*)vctKey[0]) = 0;
		*((DataValueLong*)vctVal[0]) = 100LL;
		LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
		key = lr->GetKey();
		bool bFind;
		BOOST_TEST(0 == lp->SearchRecord(*lr, bFind));
		BOOST_TEST(0 == lp->SearchKey(*key, bFind));

		LeafRecord* lr2 = lp->GetRecord(*key);
		BOOST_TEST(lr->CompareTo(*lr2) == 0);
		lp->GetRecords(*key, vctLr);
		BOOST_TEST(lr->CompareTo(*(LeafRecord*)vctLr[0]) == 0);
		BOOST_TEST(1 == vctLr.size());

		lr->ReleaseRecord();
		lr2->ReleaseRecord();
		delete key;

		*((DataValueLong*)vctKey[0]) = ROW_COUNT - 1;
		*((DataValueLong*)vctVal[0]) = ROW_COUNT + 99LL;
		lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
		key = lr->GetKey();

		lr2 = lp->GetLastRecord();
		BOOST_TEST(lr->CompareTo(*lr2) == 0);
		lr2->ReleaseRecord();

		BOOST_TEST(ROW_COUNT - 1 == lp->SearchRecord(*lr, bFind));
		BOOST_TEST(ROW_COUNT - 1 == lp->SearchKey(*key, bFind));

		lr2 = lp->GetRecord(*key);
		BOOST_TEST(lr->CompareTo(*lr2) == 0);
		lp->GetRecords(*key, vctLr);
		BOOST_TEST(lr->CompareTo(*(LeafRecord*)vctLr[0]) == 0);
		BOOST_TEST(1 == vctLr.size());

		lr->ReleaseRecord();
		lr2->ReleaseRecord();
		delete key;

		*((DataValueLong*)vctKey[0]) = ROW_COUNT / 2;
		*((DataValueLong*)vctVal[0]) = ROW_COUNT / 2 + 100LL;
		lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
		key = lr->GetKey();
		BOOST_TEST(ROW_COUNT / 2 == lp->SearchRecord(*lr, bFind));
		BOOST_TEST(ROW_COUNT / 2 == lp->SearchKey(*key, bFind));

		lr2 = lp->GetRecord(*key);
		BOOST_TEST(lr->CompareTo(*lr2) == 0);
		lp->GetRecords(*key, vctLr);
		BOOST_TEST(lr->CompareTo(*(LeafRecord*)vctLr[0]) == 0);
		BOOST_TEST(1 == vctLr.size());

		lr->ReleaseRecord();
		lr2->ReleaseRecord();
		delete key;

		delete dvKey;
		delete dvVal;
		StoragePool::SetWriteSuspend(false);
		lp->DecRefCount();
		indexTree->Close(true);
		PageBufferPool::ClearPool();
		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(LeafPageSaveLoad_test) {
		const string FILE_NAME = "./dbTest/testLeafPageSaveLoad" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = LeafPage::MAX_DATA_LENGTH / 34;

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
		LeafPage* lp = (LeafPage*)indexTree->AllocateNewPage(UINT64_MAX, (Byte)0);

		vctKey.push_back(dvKey->CloneDataValue());
		vctVal.push_back(dvVal->CloneDataValue());

		lp->WriteLock();
		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100LL;
			LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1ULL);
			lp->InsertRecord(lr, (i % 2 == 0 ? -1 : i));
		}
		lp->WriteUnlock();

		lp->SaveRecords();

		lp->LoadRecords();
		for (int i = 0; i < ROW_COUNT; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			RawKey key(vctKey);
			bool bFind;
			int pos = lp->SearchKey(key, bFind);
			BOOST_TEST(bFind);
			BOOST_TEST(pos == i);
		}

		lp->DecRefCount();
		indexTree->Close(true);
		PageBufferPool::ClearPool();
		delete dvKey;
		delete dvVal;
		
		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_CASE(LeafPageDivide_test)
	{
		const string FILE_NAME = "./dbTest/testLeafPageSaveLoad" + utils::StrMSTime() + ".dat";
		const string TABLE_NAME = "testTable";
		const int ROW_COUNT = LeafPage::MAX_DATA_LENGTH;

		PageDividePool::SetThreadStatus(true);
		StoragePool::SetWriteSuspend(true);

		DataValueLong* dvKey = new DataValueLong(100, true);
		DataValueLong* dvVal = new DataValueLong(200, false);
		VectorDataValue vctKey = { dvKey->CloneDataValue() };
		VectorDataValue vctVal = { dvVal->CloneDataValue() };
		IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
		HeadPage* hp = indexTree->GetHeadPage();
		hp->WriteIndexType(IndexType::PRIMARY);
		LeafPage* lp = (LeafPage*)indexTree->GetPage(0, true);

		vctKey.push_back(dvKey->CloneDataValue());
		vctVal.push_back(dvVal->CloneDataValue());

		for (int i = ROW_COUNT * 9; i < ROW_COUNT * 10; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100LL;
			LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal, hp->GetAndIncRecordStamp());
			lp->InsertRecord(rr);
		}

		bool b = lp->PageDivide();
		BOOST_TEST(true == b);

		LeafPage* lpNext = nullptr;
		VectorLeafRecord vlr;
		int count = ROW_COUNT * 9;

		while (lp != nullptr) {
			//LOG_DEBUG << "ID:" << lp->GetPageId() << "  RecNum:" << lp->GetRecordNum()
			//	<< "  PrevPage:" << lp->GetPrevPageId() << "  NextPage:" << lp->GetNextPageId();
			lp->GetAllRecords(vlr);

			for (LeafRecord* lr : vlr) {
				VectorDataValue vKey, vVal;
				lr->GetListKey(vKey);
				lr->GetListValue(vVal);

				BOOST_TEST(count == (int64_t)(*(DataValueLong*)vKey[0]));
				BOOST_TEST((count + 100) == (int64_t)(*(DataValueLong*)vVal[0]));
				count++;
			}

			uint64_t lNext = lp->GetNextPageId();
			lp->DecRefCount();
			if (lNext == HeadPage::NO_NEXT_PAGE_POINTER) {
				break;
			}

			lpNext = (LeafPage*)indexTree->GetPage(lNext, true);
			int rn = lpNext->GetRecordNum();
			BOOST_TEST(rn > 0);

			lp = lpNext;
		}

		BOOST_TEST(ROW_COUNT * 10 == count);

		lp = (LeafPage*)indexTree->GetPage(0, true);

		for (int i = 0; i < ROW_COUNT * 9; i++) {
			*((DataValueLong*)vctKey[0]) = i;
			*((DataValueLong*)vctVal[0]) = i + 100LL;
			LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal, hp->GetAndIncRecordStamp());
			lp->InsertRecord(rr);
		}

		b = lp->PageDivide();
		BOOST_TEST(true == b);

		count = 0;
		while (lp != nullptr) {
			//LOG_DEBUG << "ID:" << lp->GetPageId() << "  RecNum:" << lp->GetRecordNum()
			//	<< "  PrevPage:" << lp->GetPrevPageId() << "  NextPage:" << lp->GetNextPageId();
			lp->GetAllRecords(vlr);

			for (LeafRecord* lr : vlr) {
				VectorDataValue vKey, vVal;
				lr->GetListKey(vKey);
				lr->GetListValue(vVal);

				int64_t val = (int64_t)(*(DataValueLong*)vKey[0]);
				BOOST_TEST(count == (int64_t)(*(DataValueLong*)vKey[0]));
				BOOST_TEST((count + 100) == (int64_t)(*(DataValueLong*)vVal[0]));
				count++;
			}

			uint64_t lNext = lp->GetNextPageId();
			lp->DecRefCount();
			if (lNext == HeadPage::NO_NEXT_PAGE_POINTER) {
				break;
			}

			lpNext = (LeafPage*)indexTree->GetPage(lNext, true);
			BOOST_TEST(lpNext->GetRecordNum() > 0);
			
			lp = lpNext;
		}

		BOOST_TEST(ROW_COUNT * 10 == count);

		PageDividePool::SetThreadStatus(false);
		StoragePool::SetWriteSuspend(false);

		indexTree->Close(true);
		delete dvKey;
		delete dvVal;
		PageBufferPool::ClearPool();
		std::filesystem::remove(std::filesystem::path(FILE_NAME));
	}

	BOOST_AUTO_TEST_SUITE_END()
}