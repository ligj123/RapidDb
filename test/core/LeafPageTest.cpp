#include "../../src/core/LeafPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafRecord.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/pool/PageDividePool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)

BOOST_AUTO_TEST_CASE(LeafPage_test) {
  const string FILE_NAME = ROOT_PATH + "/testLeafPage" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 100;

  PageBufferPool::RemoveTimerTask();
  PageDividePool::RemoveTimerTask();
  StoragePool::RemoveTimerTask();

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueFixChar *dvVal =
      new DataValueFixChar("1234567890abcdefghijklmn", 24, 100);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1000,
                         IndexType::PRIMARY);
  LeafPage *lp = (LeafPage *)indexTree->AllocateNewPage(1, (Byte)0);

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));
  RawKey *key = new RawKey(vctKey);
  bool find;
  int32_t pos = lp->SearchKey(*key, find);
  BOOST_TEST(!find);
  delete key;

  lp->WriteLock();
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
    lp->InsertRecord(lr, i);
  }
  lp->WriteUnlock();
  lp->SaveRecords();

  *((DataValueLong *)vctKey[0]) = 0;
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  key = lr->GetKey();
  bool bFind;
  BOOST_TEST(0 == lp->SearchRecord(*lr, bFind));
  BOOST_TEST(0 == lp->SearchKey(*key, bFind));

  LeafRecord *lr2 = lp->GetRecord(0);
  BOOST_TEST(lr->CompareTo(*lr2) == 0);

  lr->DecRef();
  lr2->DecRef();
  delete key;

  *((DataValueLong *)vctKey[0]) = ROW_COUNT - 1;
  lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  key = lr->GetKey();

  lr2 = lp->GetRecord(ROW_COUNT - 1);
  BOOST_TEST(lr->CompareTo(*lr2) == 0);

  BOOST_TEST(ROW_COUNT - 1 == lp->SearchRecord(*lr, bFind));
  BOOST_TEST(ROW_COUNT - 1 == lp->SearchKey(*key, bFind));

  lr->DecRef();
  lr2->DecRef();
  delete key;

  *((DataValueLong *)vctKey[0]) = ROW_COUNT / 2;
  lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  key = lr->GetKey();
  BOOST_TEST(ROW_COUNT / 2 == lp->SearchRecord(*lr, bFind));
  BOOST_TEST(ROW_COUNT / 2 == lp->SearchKey(*key, bFind));

  lr->DecRef();
  delete key;

  delete dvKey;
  delete dvVal;
  lp->DecRef();
  indexTree->Close();

  StoragePool::AddTimerTask();
  PageDividePool::AddTimerTask();
  PageBufferPool::AddTimerTask();
}

BOOST_AUTO_TEST_CASE(LeafPageSaveLoad_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testLeafPageSaveLoad" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = LeafPage::MAX_DATA_LENGTH_LEAF / 100;

  PageBufferPool::RemoveTimerTask();
  PageDividePool::RemoveTimerTask();
  StoragePool::RemoveTimerTask();

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1001,
                         IndexType::PRIMARY);
  LeafPage *lp =
      (LeafPage *)indexTree->AllocateNewPage(PAGE_NULL_POINTER, (Byte)0);

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));

  lp->WriteLock();
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
    lp->InsertRecord(lr, i);
  }
  PageDividePool::AddCachePage(lp, true);
  lp->WriteUnlock();
  lp->DecRef();

  // Below code is to wait indexTree has been destory.
  atomic_bool bFin = false;
  indexTree->Close([&bFin]() { bFin.store(true, memory_order_relaxed); });
  PageDividePool::PoolManage();
  StoragePool::PoolManage();
  PageBufferPool::PoolManage();
  while (!bFin.load()) {
    std::this_thread::yield();
  }

  indexTree = new IndexTree();
  indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1001);
  lp = new LeafPage(indexTree, 1);
  indexTree->IncPages();
  lp->DecRef();
  lp->ReadPage();
  lp->LoadRecords();
  vctKey.push_back(dvKey->Clone(true));
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    RawKey key(vctKey);
    bool bFind;
    int pos = lp->SearchKey(key, bFind);
    BOOST_TEST(bFind);
    BOOST_TEST(pos == i);
  }

  lp->DecRef();
  indexTree->Close();
  delete dvKey;
  delete dvVal;

  StoragePool::AddTimerTask();
  PageDividePool::AddTimerTask();
  PageBufferPool::AddTimerTask();
}

BOOST_AUTO_TEST_CASE(LeafPageDivide_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testLeafPageSaveLoad" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = IndexPage::MAX_DATA_LENGTH_LEAF / 10;
  PageBufferPool::RemoveTimerTask();
  PageDividePool::RemoveTimerTask();
  StoragePool::RemoveTimerTask();

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1002,
                         IndexType::PRIMARY);
  HeadPage *hp = indexTree->GetHeadPage();
  LeafPage *lp = (LeafPage *)indexTree->GetPage(0, true);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  for (int i = ROW_COUNT * 9; i < ROW_COUNT * 10; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal,
                                    hp->GetAndIncRecordStamp(), nullptr);
    lp->InsertRecord(rr);
  }

  bool b = lp->PageDivide();
  BOOST_TEST(true == b);
  lp->IncRef();

  LeafPage *lpNext = nullptr;
  VectorLeafRecord vlr;
  int count = ROW_COUNT * 9;

  while (lp != nullptr) {
    // LOG_DEBUG << "ID:" << lp->GetPageId() << "  RecNum:" <<
    // lp->GetRecordNum()
    //  << "  PrevPage:" << lp->GetPrevPageId() << "  NextPage:" <<
    //  lp->GetNextPageId();
    lp->GetAllRecords(vlr);

    for (LeafRecord *lr : vlr) {
      VectorDataValue vVal;
      *((DataValueLong *)vctKey[0]) = count;
      RawKey key(vctKey);
      BOOST_TEST(lr->CompareKey(key) == 0);

      lr->GetListValue(vVal);
      BOOST_TEST((count + 100) == (int64_t)(*(DataValueLong *)vVal[0]));
      count++;
    }

    uint32_t lNext = lp->GetNextPageId();
    lp->DecRef();
    if (lNext == PAGE_NULL_POINTER) {
      break;
    }

    lpNext = (LeafPage *)indexTree->GetPage(lNext, true);
    int rn = lpNext->GetRecordNumber();
    BOOST_TEST(rn > 0);

    lp = lpNext;
  }

  BOOST_TEST(ROW_COUNT * 10 == count);

  lp = (LeafPage *)indexTree->GetPage(0, true);

  for (int i = 0; i < ROW_COUNT * 9; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal,
                                    hp->GetAndIncRecordStamp(), nullptr);
    lp->InsertRecord(rr);
  }

  b = lp->PageDivide();
  BOOST_TEST(true == b);

  lp->IncRef();
  count = 0;
  while (lp != nullptr) {
    // LOG_DEBUG << "ID:" << lp->GetPageId() << "  RecNum:" <<
    // lp->GetRecordNum()
    //  << "  PrevPage:" << lp->GetPrevPageId() << "  NextPage:" <<
    //  lp->GetNextPageId();
    lp->GetAllRecords(vlr);
    for (LeafRecord *lr : vlr) {
      VectorDataValue vVal;
      *((DataValueLong *)vctKey[0]) = count;
      RawKey key(vctKey);
      BOOST_TEST(lr->CompareKey(key) == 0);

      lr->GetListValue(vVal);
      BOOST_TEST((count + 100) == (int64_t)(*(DataValueLong *)vVal[0]));
      count++;
    }

    uint32_t lNext = lp->GetNextPageId();
    lp->DecRef();
    if (lNext == PAGE_NULL_POINTER) {
      break;
    }

    lpNext = (LeafPage *)indexTree->GetPage(lNext, true);
    BOOST_TEST(lpNext->GetRecordNumber() > 0U);

    lp = lpNext;
  }

  BOOST_TEST(ROW_COUNT * 10 == count);

  indexTree->Close();
  delete dvKey;
  delete dvVal;

  StoragePool::AddTimerTask();
  PageDividePool::AddTimerTask();
  PageBufferPool::AddTimerTask();
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
