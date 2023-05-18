#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueDigit.h"
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

BOOST_AUTO_TEST_CASE(IndexTreeInsertRecord_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexTreeInsertRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 1000;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  bool rt = indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3000,
                                   IndexType::PRIMARY);
  BOOST_TEST(rt);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *rr =
        new LeafRecord(indexTree, vctKey, vctVal,
                       indexTree->GetHeadPage()->ReadRecordStamp(), nullptr);
    IndexPage *idxPage = nullptr;
    bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    ((LeafPage *)idxPage)->InsertRecord(rr, false);
    PageDividePool::AddPage(idxPage, false);
    idxPage->WriteUnlock();
  }

  IndexTree::TestCloseWait(indexTree);

  indexTree = new IndexTree();
  rt = indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3000);
  BOOST_TEST(rt);
  LeafPage *lp = indexTree->GetBeginPage();
  uint64_t idx = 0;
  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  while (true) {
    for (uint32_t i = 0; i < lp->GetRecordNumber(); i++) {
      LeafRecord *lr = lp->GetRecord(i);
      *((DataValueLong *)vctKey[0]) = idx;
      RawKey key(vctKey);
      BOOST_TEST(lr->CompareKey(key) == 0);

      VectorDataValue vdv;
      lr->GetListValue(vdv);
      BOOST_TEST(vdv[0]->GetLong() == (idx + 100));
      lr->DecRef();
      idx++;
    }

    PageID nid = lp->GetNextPageId();
    if (nid == PAGE_NULL_POINTER)
      break;

    LeafPage *lp2 =
        (LeafPage *)indexTree->GetPage(nid, PageType::LEAF_PAGE, true);
    lp->DecRef();
    lp = lp2;
  }

  lp->DecRef();
  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToNonUniqueIndex_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexRepeatedKey" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 3000;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  bool rt = indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3001,
                                   IndexType::NON_UNIQUE);
  BOOST_TEST(rt);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  Byte bys[100];

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
    Int64ToBytes(i + 100, bys, true);
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                    ActionType::INSERT, nullptr);
    IndexPage *idxPage = nullptr;
    bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);
    ((LeafPage *)idxPage)->InsertRecord(rr);
    PageDividePool::AddPage(idxPage, false);
    idxPage->WriteUnlock();
  }

  IndexTree::TestCloseWait(indexTree);

  indexTree = new IndexTree();
  rt = indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3002);
  BOOST_TEST(rt);
  LeafPage *lp = indexTree->GetBeginPage();
  uint64_t idx = 0;
  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  while (true) {
    for (uint32_t i = 0; i < lp->GetRecordNumber(); i++) {
      LeafRecord *lr = lp->GetRecord(i);
      *((DataValueLong *)vctKey[0]) = idx / 3;
      RawKey key(vctKey);
      BOOST_TEST(lr->CompareKey(key) == 0);

      RawKey *pkey = lr->GetPrimayKey();
      *((DataValueLong *)vctKey[0]) =
          idx / 3 + (idx % 3) * (ROW_COUNT / 3) + 100;
      RawKey key2(vctKey);

      BOOST_TEST(key2.CompareTo(*pkey) == 0);
      delete pkey;
      lr->DecRef();
      idx++;
    }

    PageID nid = lp->GetNextPageId();
    if (nid == PAGE_NULL_POINTER)
      break;

    LeafPage *lp2 =
        (LeafPage *)indexTree->GetPage(nid, PageType::LEAF_PAGE, true);
    lp->DecRef();
    lp = lp2;
  }

  lp->DecRef();
  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToPrimaryKey_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexRepeatedKey" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 30000;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3002,
                         IndexType::PRIMARY);

  vctKey.push_back(new DataValueLong(10));
  vctVal.push_back(new DataValueLong(100));

  IndexPage *idxPage = nullptr;
  LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, 0, nullptr);
  indexTree->SearchRecursively(*rr, true, idxPage, true);
  bool b = ((LeafPage *)idxPage)->InsertRecord(rr, false);
  BOOST_TEST(b);

  *((DataValueLong *)vctVal[0]) = 200;
  rr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  b = ((LeafPage *)idxPage)->InsertRecord(rr, false);
  BOOST_TEST(!b);
  BOOST_TEST(_threadErrorMsg->getErrId() == CORE_REPEATED_RECORD);

  rr->DecRef();
  idxPage->DecRef();
  idxPage->WriteUnlock();
  PageDividePool::AddPage(idxPage, false);

  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedRecordToNonUniqueIndex_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexRepeatedRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3003,
                         IndexType::NON_UNIQUE);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  Byte bys[100];

  *((DataValueLong *)vctKey[0]) = 10;
  Int64ToBytes(100, bys, true);
  LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                  ActionType::INSERT, nullptr);
  IndexPage *idxPage = nullptr;
  bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
  BOOST_TEST(b);
  b = ((LeafPage *)idxPage)->InsertRecord(rr, false);
  BOOST_TEST(b);

  LeafRecord *rr2 = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                   ActionType::INSERT, nullptr);
  b = ((LeafPage *)idxPage)->InsertRecord(rr2, false);
  BOOST_TEST(!b);
  BOOST_TEST(_threadErrorMsg->getErrId() == CORE_REPEATED_RECORD);
  rr2->DecRef();

  Int64ToBytes(200, bys, true);
  LeafRecord *rr3 = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                   ActionType::INSERT, nullptr);
  b = ((LeafPage *)idxPage)->InsertRecord(rr3, false);
  BOOST_TEST(b);

  PageDividePool::AddPage(idxPage, false);
  idxPage->WriteUnlock();

  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeUniqueIndex_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexUniqueRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3004,
                         IndexType::UNIQUE);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  int64_t arr[] = {9, 3, 5, 8, 7, 2, 4, 0, 6, 1};
  Byte bys[100];

  for (int i = 0; i < 10; i++) {
    *((DataValueLong *)vctKey[0]) = arr[i];
    Int64ToBytes(i + 100, bys, true);
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, UI64_LEN,
                                    ActionType::INSERT, nullptr);

    IndexPage *idxPage = nullptr;
    bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    ((LeafPage *)idxPage)->InsertRecord(rr);
    PageDividePool::AddPage(idxPage, false);
    idxPage->WriteUnlock();
  }

  IndexTree::TestCloseWait(indexTree);

  indexTree = new IndexTree();
  indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3004);
  vctKey.push_back(dvKey->Clone());
  LeafPage *lp = indexTree->GetBeginPage();
  lp->WriteLock();

  for (int i = 0; i < 10; i++) {
    *((DataValueLong *)vctKey[0]) = arr[i];
    RawKey key(vctKey);
    bool bfind;
    int32_t pos = lp->SearchKey(key, bfind);
    LeafRecord *lr = lp->GetRecord(pos);
    BOOST_TEST(lr->CompareKey(key) == 0);

    *((DataValueLong *)vctKey[0]) = i + 100;
    RawKey key2(vctKey);
    RawKey *pkey = lr->GetPrimayKey();
    BOOST_TEST(pkey->CompareTo(key2) == 0);
    delete pkey;
    lr->DecRef();
  }

  *((DataValueLong *)vctKey[0]) = arr[2];
  Int64ToBytes(100, bys, true);
  LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, UI64_LEN,
                                  ActionType::INSERT, nullptr);
  bool b = lp->InsertRecord(rr);
  BOOST_TEST(!b);
  BOOST_TEST(_threadErrorMsg->getErrId() == CORE_REPEATED_RECORD);
  rr->DecRef();

  lp->DecRef();
  lp->WriteUnlock();
  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeGetRecordWithNonUniqueIndex_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexGetRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 6000;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3005,
                         IndexType::NON_UNIQUE);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  Byte bys[100];

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
    Int64ToBytes(100 + i, bys, true);
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                    ActionType::INSERT, nullptr);
    IndexPage *idxPage = nullptr;
    bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    ((LeafPage *)idxPage)->InsertRecord(rr, false);
    PageDividePool::AddPage(idxPage, false);
    idxPage->WriteUnlock();
  }

  IndexTree::TestCloseWait(indexTree);

  indexTree = new IndexTree();
  bool b = indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3005);
  BOOST_TEST(b);

  vctKey.push_back(dvKey->Clone());

  for (int i = 0; i < ROW_COUNT / 3; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    RawKey key(vctKey);

    IndexPage *idp = nullptr;
    bool b = indexTree->SearchRecursively(key, false, idp, true);
    BOOST_TEST(b);
    BOOST_TEST(idp->GetPageType() == PageType::LEAF_PAGE);

    LeafPage *lp = (LeafPage *)idp;
    bool bFind;
    int32_t pos = lp->SearchKey(key, bFind);
    BOOST_TEST(bFind);

    for (uint32_t j = 0; j < 3; j++) {
      if (pos >= (int32_t)lp->GetRecordNumber()) {
        PageID nid = lp->GetNextPageId();
        lp->ReadUnlock();
        lp->DecRef();
        lp = (LeafPage *)indexTree->GetPage(nid, PageType::LEAF_PAGE, true);
        lp->ReadLock();
        pos = 0;
      }
      LeafRecord *lr = lp->GetRecord(pos);
      BOOST_TEST(lr->CompareKey(key) == 0);

      RawKey *pkey = lr->GetPrimayKey();
      *((DataValueLong *)vctKey[0]) = i + j * (ROW_COUNT / 3) + 100;
      RawKey key2(vctKey);
      BOOST_TEST(key2.CompareTo(*pkey) == 0);
      delete pkey;
      lr->DecRef();
      pos++;
    }

    lp->DecRef();
    lp->ReadUnlock();
  }

  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeQueryRecordWithPrimaryKey_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexRepeatedRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 10000;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3006,
                         IndexType::PRIMARY);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *rr =
        new LeafRecord(indexTree, vctKey, vctVal,
                       indexTree->GetHeadPage()->ReadRecordStamp(), nullptr);

    IndexPage *idxPage = nullptr;
    bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    ((LeafPage *)idxPage)->InsertRecord(rr, false);
    PageDividePool::AddPage(idxPage, false);
    idxPage->WriteUnlock();
  }

  IndexTree::TestCloseWait(indexTree);

  indexTree = new IndexTree();
  bool b = indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3005);
  BOOST_TEST(b);

  vctKey.push_back(dvKey->Clone());

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
    RawKey key(vctKey);

    IndexPage *idp = nullptr;
    bool b = indexTree->SearchRecursively(key, false, idp, true);
    BOOST_TEST(b);
    BOOST_TEST(idp->GetPageType() == PageType::LEAF_PAGE);

    LeafPage *lp = (LeafPage *)idp;
    bool bFind;
    int32_t pos = lp->SearchKey(key, bFind);
    BOOST_TEST(bFind);

    LeafRecord *lr = lp->GetRecord(pos);
    BOOST_TEST(lr->CompareKey(key) == 0);

    int rt = lr->GetListValue(vctVal);
    BOOST_TEST(rt == 0);
    BOOST_TEST(vctVal[0]->GetLong() == i + 100LL);
    lr->DecRef();

    lp->DecRef();
    lp->ReadUnlock();
  }

  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeReadPrimaryKeysUnique_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testIndexReadPrimaryKeys" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 6000;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3006,
                         IndexType::UNIQUE);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  Byte bys[100];

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
    Int64ToBytes(100 + i, bys, true);
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                    ActionType::INSERT, nullptr);

    IndexPage *idxPage = nullptr;
    bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    ((LeafPage *)idxPage)->InsertRecord(rr, false);
    PageDividePool::AddPage(idxPage, false);
    idxPage->WriteUnlock();
  }

  IndexTree::TestCloseWait(indexTree);

  indexTree = new IndexTree();
  indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 3006);
  vctKey.push_back(dvKey->Clone());

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
    RawKey key(vctKey);

    IndexPage *idp = nullptr;
    bool b = indexTree->SearchRecursively(key, false, idp, true);
    BOOST_TEST(b);
    BOOST_TEST(idp->GetPageType() == PageType::LEAF_PAGE);

    LeafPage *lp = (LeafPage *)idp;
    bool bFind;
    int32_t pos = lp->SearchKey(key, bFind);
    BOOST_TEST(bFind);

    LeafRecord *lr = lp->GetRecord(pos);
    BOOST_TEST(lr->CompareKey(key) == 0);

    RawKey *pkey = lr->GetPrimayKey();
    *((DataValueLong *)vctKey[0]) = i + 100;
    RawKey key2(vctKey);
    BOOST_TEST(key2.CompareTo(*pkey) == 0);
    delete pkey;
    lr->DecRef();

    lp->DecRef();
    lp->ReadUnlock();
  }

  IndexTree::TestCloseWait(indexTree);
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
