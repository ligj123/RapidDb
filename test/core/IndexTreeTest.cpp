#include "../../src/core/IndexTree.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/BranchRecord.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/pool/FilePagePool.h"
#include "../../src/utils/BytesFuncs.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_AUTO_TEST_SUITE(CoreTest)

BOOST_AUTO_TEST_CASE(IndexTreeInsertRecord_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testIndexTreeInsertRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 1000;
  MTreeMap<uint64_t, CachePage *> pageMap;
  ThreadPool::SetThreadId(0);
  FilePagePool::Start(1);

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  bool rt =
      indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                                 vctVal, GetFileId(), IndexType::PRIMARY);
  BOOST_TEST(rt);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal,
                                    indexTree->GetHeadPage()->GetRecordStamp(),
                                    nullptr, false);
    IndexPage *idxPage = indexTree->GetRootPage();
    bool b = indexTree->SearchPage(*rr, idxPage);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    LeafPage *lp = (LeafPage *)idxPage;
    bool bFind;
    int32_t pos = lp->SearchRecord(*rr, bFind);
    BOOST_TEST(!bFind);
    lp->InsertRecord(rr, pos);
    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap, UINT8_MAX);
    }
  }

  indexTree->SettleUpdatedPages(pageMap);
  assert(pageMap.size() == 0);

  IndexPage *root = indexTree->GetRootPage();
  MDeque<IndexPage *> queue;
  queue.push_back(root);

  while (queue.size() > 0) {
    IndexPage *page = queue.front();
    queue.pop_front();

    while (page->GetPageStatus() != PageStatus::VALID) {
      this_thread::yield();
    }

    if (page->GetPageType() == PageType::BRANCH_PAGE) {
      BranchPage *bpage = (BranchPage *)page;
      for (uint32_t i = 0; i < bpage->GetRecordNumber(); i++) {
        BranchRecord *br = bpage->GetVctRecord(i);
        queue.push_back(br->GetChildPage());
      }
    }

    page->SetReferred(false);
  }

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

  indexTree = new IndexTree();
  rt = indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                                vctVal, GetFileId());
  BOOST_TEST(rt);
  LeafPage *lp = indexTree->GetBeginPage();
  uint64_t idx = 0;
  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  while (true) {
    while (lp->GetPageStatus() == PageStatus::READING) {
      this_thread::yield();
    }

    lp->SetPageStatus(PageStatus::VALID, memory_order_acquire);

    for (uint32_t i = 0; i < lp->GetRecordNumber(); i++) {
      LeafRecord &lr = lp->GetRecord(i);
      *((DataValueLong *)vctKey[0]) = idx;
      RawKey key(vctKey);
      BOOST_TEST(lr.CompareKey(key) == 0);

      VectorDataValue vdv;
      lr.ReadListValue({}, vdv, indexTree);
      BOOST_TEST(vdv[0]->GetLong() == (idx + 100));
      idx++;
    }

    PageID pid = lp->GetNextPageId();
    lp->SetReferred(false);
    if (pid == PAGE_NULL_POINTER)
      break;

    lp = (LeafPage *)indexTree->GetPage(pid, PageType::LEAF_PAGE);
  }

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

  delete dvKey;
  delete dvVal;
  FilePagePool::Stop();
}

BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToNonUniqueIndex_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testIndexRepeatedKey" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 3000;

  MTreeMap<uint64_t, CachePage *> pageMap;
  ThreadPool::SetThreadId(0);
  FilePagePool::Start(1);

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  bool rt =
      indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                                 vctVal, GetFileId(), IndexType::NON_UNIQUE);
  BOOST_TEST(rt);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  Byte bys[100];

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
    Int64ToBytes(i + 100, bys, true);
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                    ActionType::INSERT,
                                    indexTree->GetHeadPage()->GetRecordStamp());
    IndexPage *idxPage = indexTree->GetRootPage();
    bool b = indexTree->SearchPage(*rr, idxPage);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    LeafPage *lp = (LeafPage *)idxPage;
    bool bFind;
    int32_t pos = lp->SearchRecord(*rr, bFind);
    BOOST_TEST(!bFind);
    lp->InsertRecord(rr, pos);
    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap, UINT8_MAX);
    }
  }

  indexTree->SettleUpdatedPages(pageMap);
  assert(pageMap.size() == 0);

  IndexPage *root = indexTree->GetRootPage();
  MDeque<IndexPage *> queue;
  queue.push_back(root);

  while (queue.size() > 0) {
    IndexPage *page = queue.front();
    queue.pop_front();

    while (page->GetPageStatus() != PageStatus::VALID) {
      this_thread::yield();
    }

    if (page->GetPageType() == PageType::BRANCH_PAGE) {
      BranchPage *bpage = (BranchPage *)page;
      for (uint32_t i = 0; i < bpage->GetRecordNumber(); i++) {
        BranchRecord *br = bpage->GetVctRecord(i);
        queue.push_back(br->GetChildPage());
      }
    }

    page->SetReferred(false);
  }

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

  indexTree = new IndexTree();
  rt = indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                                vctVal, GetFileId());
  BOOST_TEST(rt);
  LeafPage *lp = indexTree->GetBeginPage();
  uint64_t idx = 0;
  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  while (true) {
    while (lp->GetPageStatus() == PageStatus::READING) {
      this_thread::yield();
    }

    lp->SetPageStatus(PageStatus::VALID, memory_order_acquire);

    for (uint32_t i = 0; i < lp->GetRecordNumber(); i++) {
      LeafRecord &lr = lp->GetRecord(i);
      *((DataValueLong *)vctKey[0]) = idx / 3;
      RawKey key(vctKey);
      BOOST_TEST(lr.CompareKey(key) == 0);

      const RawKey &pkey = lr.GetPrimayKey();
      *((DataValueLong *)vctKey[0]) =
          idx / 3 + (idx % 3) * (ROW_COUNT / 3) + 100;
      RawKey key2(vctKey);

      BOOST_TEST(key2.CompareTo(pkey) == 0);
      idx++;
    }

    PageID pid = lp->GetNextPageId();
    lp->SetReferred(false);
    if (pid == PAGE_NULL_POINTER)
      break;

    lp = (LeafPage *)indexTree->GetPage(pid, PageType::LEAF_PAGE);
  }

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

  delete dvKey;
  delete dvVal;
  FilePagePool::Stop();
}

BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToPrimaryKey_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testIndexRepeatedKey" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, GetFileId(), IndexType::PRIMARY);

  vctKey.push_back(new DataValueLong(10));
  vctVal.push_back(new DataValueLong(100));

  IndexPage *idxPage = indexTree->GetRootPage();
  LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, 0, nullptr, false);
  indexTree->SearchPage(*rr, idxPage);

  LeafPage *lp = (LeafPage *)idxPage;
  bool bFind;
  int32_t pos = lp->SearchRecord(*rr, bFind);
  BOOST_TEST(!bFind);
  lp->InsertRecord(rr, pos);

  *((DataValueLong *)vctVal[0]) = 200;
  rr = new LeafRecord(indexTree, vctKey, vctVal, 2, nullptr, false);
  pos = lp->SearchRecord(*rr, bFind);
  BOOST_TEST(bFind);
  delete rr;

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

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
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, GetFileId(), IndexType::NON_UNIQUE);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  Byte bys[100];

  *((DataValueLong *)vctKey[0]) = 10;
  Int64ToBytes(100, bys, true);
  LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                  ActionType::INSERT, 1);
  IndexPage *idxPage = indexTree->GetRootPage();
  bool b = indexTree->SearchPage(*rr, idxPage);
  BOOST_TEST(b);

  LeafPage *lp = (LeafPage *)idxPage;
  bool bFind;
  int32_t pos = lp->SearchRecord(*rr, bFind);
  BOOST_TEST(!bFind);
  lp->InsertRecord(rr, pos);

  LeafRecord *rr2 = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                   ActionType::INSERT, 2);
  pos = lp->SearchRecord(*rr2, bFind);
  BOOST_TEST(bFind);
  delete rr2;

  Int64ToBytes(200, bys, true);
  LeafRecord *rr3 = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
                                   ActionType::INSERT, 3);
  pos = lp->SearchRecord(*rr2, bFind);
  BOOST_TEST(!bFind);
  lp->InsertRecord(rr3, pos);

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(IndexTreeUniqueIndex_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testIndexUniqueRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  MTreeMap<uint64_t, CachePage *> pageMap;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, 3004, IndexType::UNIQUE);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());
  int64_t arr[] = {9, 3, 5, 8, 7, 2, 4, 0, 6, 1};
  Byte bys[100];

  for (int i = 0; i < 10; i++) {
    *((DataValueLong *)vctKey[0]) = arr[i];
    Int64ToBytes(i + 100, bys, true);
    LeafRecord *rr =
        new LeafRecord(indexTree, vctKey, bys, UI64_LEN, ActionType::INSERT,
                       indexTree->GetHeadPage()->GetRecordStamp());

    IndexPage *idxPage = indexTree->GetRootPage();
    bool b = indexTree->SearchPage(*rr, idxPage);
    BOOST_TEST(b);
    BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

    LeafPage *lp = (LeafPage *)idxPage;
    bool bFind;
    int32_t pos = lp->SearchRecord(*rr, bFind);
    BOOST_TEST(!bFind);
    lp->InsertRecord(rr, pos);

    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap, UINT8_MAX);
    }
  }

  LeafPage *lp = (LeafPage *)indexTree->GetRootPage();
  lp->SaveRecords(pageMap, false);
  FilePagePool::SyncWritePage(lp);

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

  indexTree = new IndexTree();
  indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                           vctVal, GetFileId());
  vctKey.push_back(dvKey->Clone());
  lp = indexTree->GetBeginPage();

  for (int i = 0; i < 10; i++) {
    *((DataValueLong *)vctKey[0]) = arr[i];
    RawKey key(vctKey);
    bool bfind;
    int32_t pos = lp->SearchKey(key, bfind);
    BOOST_TEST(bfind);
    LeafRecord &lr = lp->GetRecord(pos);
    BOOST_TEST(lr.CompareKey(key) == 0);

    *((DataValueLong *)vctKey[0]) = i + 100;
    RawKey key2(vctKey);
    RawKey pkey = lr.GetPrimayKey();
    BOOST_TEST(pkey.CompareTo(key2) == 0);
  }

  *((DataValueLong *)vctKey[0]) = arr[2];
  Int64ToBytes(100, bys, true);
  LeafRecord *rr =
      new LeafRecord(indexTree, vctKey, bys, UI64_LEN, ActionType::INSERT,
                     indexTree->GetHeadPage()->GetRecordStamp());
  bool bFind;
  int32_t pos = lp->SearchRecord(*rr, bFind);
  BOOST_TEST(bFind);
  delete rr;

  lp->SetReferred(false);
  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;
  delete dvKey;
  delete dvVal;
}

// BOOST_AUTO_TEST_CASE(IndexTreeGetRecordWithNonUniqueIndex_test) {
//   const string FILE_NAME =
//       ROOT_PATH + "/testIndexGetRecord" + StrMSTime() + ".dat";
//   const string TABLE_NAME = "testTable";
//   const int ROW_COUNT = 6000;

//   DataValueLong *dvKey = new DataValueLong(100);
//   DataValueLong *dvVal = new DataValueLong(200);
//   VectorDataValue vctKey = {dvKey->Clone()};
//   VectorDataValue vctVal = {dvVal->Clone()};
//   IndexTree *indexTree = new IndexTree();
//   indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
//                              vctVal, 3005, IndexType::NON_UNIQUE);

//   vctKey.push_back(dvKey->Clone());
//   vctVal.push_back(dvVal->Clone());
//   Byte bys[100];

//   for (int i = 0; i < ROW_COUNT; i++) {
//     *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
//     Int64ToBytes(100 + i, bys, true);
//     LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
//                                     ActionType::INSERT, nullptr);
//     IndexPage *idxPage = nullptr;
//     bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
//     BOOST_TEST(b);
//     BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

//     ((LeafPage *)idxPage)->InsertRecord(rr, false);
//     PageDividePool::AddPage(idxPage, false);
//     idxPage->WriteUnlock();
//   }

//   IndexTree::TestCloseWait(indexTree);

//   indexTree = new IndexTree();
//   bool b = indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(),
//                                     vctKey, vctVal, 3005);
//   BOOST_TEST(b);

//   vctKey.push_back(dvKey->Clone());

//   for (int i = 0; i < ROW_COUNT / 3; i++) {
//     *((DataValueLong *)vctKey[0]) = i;
//     RawKey key(vctKey);

//     IndexPage *idp = nullptr;
//     bool b = indexTree->SearchRecursively(key, false, idp, true);
//     BOOST_TEST(b);
//     BOOST_TEST(idp->GetPageType() == PageType::LEAF_PAGE);

//     LeafPage *lp = (LeafPage *)idp;
//     bool bFind;
//     int32_t pos = lp->SearchKey(key, bFind);
//     BOOST_TEST(bFind);

//     for (uint32_t j = 0; j < 3; j++) {
//       if (pos >= (int32_t)lp->GetRecordNumber()) {
//         PageID nid = lp->GetNextPageId();
//         lp->ReadUnlock();
//         lp->DecRef();
//         lp = (LeafPage *)indexTree->GetPage(nid, PageType::LEAF_PAGE, true);
//         lp->ReadLock();
//         pos = 0;
//       }
//       LeafRecord *lr = lp->GetRecord(pos);
//       BOOST_TEST(lr->CompareKey(key) == 0);

//       RawKey *pkey = lr->GetPrimayKey();
//       *((DataValueLong *)vctKey[0]) = i + j * (ROW_COUNT / 3) + 100;
//       RawKey key2(vctKey);
//       BOOST_TEST(key2.CompareTo(*pkey) == 0);
//       delete pkey;
//       lr->DecRef();
//       pos++;
//     }

//     lp->DecRef();
//     lp->ReadUnlock();
//   }

//   IndexTree::TestCloseWait(indexTree);
//   delete dvKey;
//   delete dvVal;
// }

// BOOST_AUTO_TEST_CASE(IndexTreeQueryRecordWithPrimaryKey_test) {
//   const string FILE_NAME =
//       ROOT_PATH + "/testIndexRepeatedRecord" + StrMSTime() + ".dat";
//   const string TABLE_NAME = "testTable";
//   const int ROW_COUNT = 10000;

//   DataValueLong *dvKey = new DataValueLong(100);
//   DataValueLong *dvVal = new DataValueLong(200);
//   VectorDataValue vctKey = {dvKey->Clone()};
//   VectorDataValue vctVal = {dvVal->Clone()};
//   IndexTree *indexTree = new IndexTree();
//   indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
//                              vctVal, 3006, IndexType::PRIMARY);

//   vctKey.push_back(dvKey->Clone());
//   vctVal.push_back(dvVal->Clone());
//   for (int i = 0; i < ROW_COUNT; i++) {
//     *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
//     *((DataValueLong *)vctVal[0]) = i + 100LL;
//     LeafRecord *rr =
//         new LeafRecord(indexTree, vctKey, vctVal,
//                        indexTree->GetHeadPage()->ReadRecordStamp(), nullptr);

//     IndexPage *idxPage = nullptr;
//     bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
//     BOOST_TEST(b);
//     BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

//     ((LeafPage *)idxPage)->InsertRecord(rr, false);
//     PageDividePool::AddPage(idxPage, false);
//     idxPage->WriteUnlock();
//   }

//   IndexTree::TestCloseWait(indexTree);

//   indexTree = new IndexTree();
//   bool b = indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(),
//                                     vctKey, vctVal, 3005);
//   BOOST_TEST(b);

//   vctKey.push_back(dvKey->Clone());

//   for (int i = 0; i < ROW_COUNT; i++) {
//     *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
//     RawKey key(vctKey);

//     IndexPage *idp = nullptr;
//     bool b = indexTree->SearchRecursively(key, false, idp, true);
//     BOOST_TEST(b);
//     BOOST_TEST(idp->GetPageType() == PageType::LEAF_PAGE);

//     LeafPage *lp = (LeafPage *)idp;
//     bool bFind;
//     int32_t pos = lp->SearchKey(key, bFind);
//     BOOST_TEST(bFind);

//     LeafRecord *lr = lp->GetRecord(pos);
//     BOOST_TEST(lr->CompareKey(key) == 0);

//     int rt = lr->GetListValue(vctVal);
//     BOOST_TEST(rt == 0);
//     BOOST_TEST(vctVal[0]->GetLong() == i + 100LL);
//     lr->DecRef();

//     lp->DecRef();
//     lp->ReadUnlock();
//   }

//   IndexTree::TestCloseWait(indexTree);
//   delete dvKey;
//   delete dvVal;
// }

// BOOST_AUTO_TEST_CASE(IndexTreeReadPrimaryKeysUnique_test) {
//   const string FILE_NAME =
//       ROOT_PATH + "/testIndexReadPrimaryKeys" + StrMSTime() + ".dat";
//   const string TABLE_NAME = "testTable";
//   const int ROW_COUNT = 6000;

//   DataValueLong *dvKey = new DataValueLong(100);
//   DataValueLong *dvVal = new DataValueLong(200);
//   VectorDataValue vctKey = {dvKey->Clone()};
//   VectorDataValue vctVal = {dvVal->Clone()};
//   IndexTree *indexTree = new IndexTree();
//   indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
//                              vctVal, 3006, IndexType::UNIQUE);

//   vctKey.push_back(dvKey->Clone());
//   vctVal.push_back(dvVal->Clone());
//   Byte bys[100];

//   for (int i = 0; i < ROW_COUNT; i++) {
//     *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
//     Int64ToBytes(100 + i, bys, true);
//     LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t),
//                                     ActionType::INSERT, nullptr);

//     IndexPage *idxPage = nullptr;
//     bool b = indexTree->SearchRecursively(*rr, true, idxPage, true);
//     BOOST_TEST(b);
//     BOOST_TEST(idxPage->GetPageType() == PageType::LEAF_PAGE);

//     ((LeafPage *)idxPage)->InsertRecord(rr, false);
//     PageDividePool::AddPage(idxPage, false);
//     idxPage->WriteUnlock();
//   }

//   IndexTree::TestCloseWait(indexTree);

//   indexTree = new IndexTree();
//   indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
//                            vctVal, 3006);
//   vctKey.push_back(dvKey->Clone());

//   for (int i = 0; i < ROW_COUNT; i++) {
//     *((DataValueLong *)vctKey[0]) = GenTestPrimaryKey(i);
//     RawKey key(vctKey);

//     IndexPage *idp = nullptr;
//     bool b = indexTree->SearchRecursively(key, false, idp, true);
//     BOOST_TEST(b);
//     BOOST_TEST(idp->GetPageType() == PageType::LEAF_PAGE);

//     LeafPage *lp = (LeafPage *)idp;
//     bool bFind;
//     int32_t pos = lp->SearchKey(key, bFind);
//     BOOST_TEST(bFind);

//     LeafRecord *lr = lp->GetRecord(pos);
//     BOOST_TEST(lr->CompareKey(key) == 0);

//     RawKey *pkey = lr->GetPrimayKey();
//     *((DataValueLong *)vctKey[0]) = i + 100;
//     RawKey key2(vctKey);
//     BOOST_TEST(key2.CompareTo(*pkey) == 0);
//     delete pkey;
//     lr->DecRef();

//     lp->DecRef();
//     lp->ReadUnlock();
//   }

//   IndexTree::TestCloseWait(indexTree);
//   delete dvKey;
//   delete dvVal;
// }

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
