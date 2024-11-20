#include "../../src/core/LeafPage.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/BranchRecord.h"
#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafRecord.h"
#include "../../src/dataType/DataValueFactory.h"
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

BOOST_AUTO_TEST_CASE(LeafPage_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testLeafPage" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 100;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueFixChar *dvVal =
      new DataValueFixChar("1234567890abcdefghijklmn", 24, 100);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);
  LeafPage *lp =
      (LeafPage *)indexTree.ApplyIndexPages(nullptr, (Byte)0, 1, false)[0];
  MTreeMap<uint64_t, CachePage *> pageMap;

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));
  RawKey *key = new RawKey(vctKey);
  bool find;
  int32_t pos = lp->SearchKey(*key, find);
  BOOST_TEST(!find);
  delete key;

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    LeafRecord *lr =
        new LeafRecord(&indexTree, vctKey, vctVal, i, nullptr, false);
    lp->InsertRecord(lr, i);
  }
  lp->SaveRecords(pageMap, false);
  assert(pageMap.size() == 0);

  *((DataValueLong *)vctKey[0]) = 0;
  LeafRecord *lr =
      new LeafRecord(&indexTree, vctKey, vctVal, 0, nullptr, false);
  key = new RawKey(vctKey);
  bool bFind;
  BOOST_TEST(0 == lp->SearchRecord(*lr, bFind));
  BOOST_TEST(bFind);
  BOOST_TEST(0 == lp->SearchKey(*key, bFind));
  BOOST_TEST(bFind);

  const LeafRecord &lr2 = lp->GetRecord(0);
  BOOST_TEST(lr->CompareTo(lr2) == 0);

  delete lr;
  delete key;

  *((DataValueLong *)vctKey[0]) = ROW_COUNT - 1;
  lr =
      new LeafRecord(&indexTree, vctKey, vctVal, ROW_COUNT - 1, nullptr, false);
  key = new RawKey(vctKey);

  const LeafRecord &lr3 = lp->GetRecord(ROW_COUNT - 1);
  BOOST_TEST(lr->CompareTo(lr3) == 0);

  BOOST_TEST(ROW_COUNT - 1 == lp->SearchRecord(*lr, bFind));
  BOOST_TEST(bFind);
  BOOST_TEST(ROW_COUNT - 1 == lp->SearchKey(*key, bFind));
  BOOST_TEST(bFind);

  delete lr;
  delete key;

  *((DataValueLong *)vctKey[0]) = ROW_COUNT / 2;
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
  key = new RawKey(vctKey);
  BOOST_TEST(ROW_COUNT / 2 == lp->SearchRecord(*lr, bFind));
  BOOST_TEST(ROW_COUNT / 2 == lp->SearchKey(*key, bFind));

  delete lr;
  delete key;

  delete dvKey;
  delete dvVal;
  lp->SetReferred(false);
  indexTree.Close();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(LeafPageSaveLoad_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testLeafPageSaveLoad" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = LeafPage::MAX_DATA_LENGTH_LEAF / 100;
  MTreeMap<uint64_t, CachePage *> pageMap;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  int32_t fileId = GetFileId();
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, fileId, IndexType::PRIMARY);
  LeafPage *lp =
      (LeafPage *)indexTree->ApplyIndexPages(nullptr, (Byte)0, 1, false)[0];

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
    lp->InsertRecord(lr, i);
  }

  lp->SaveRecords(pageMap, false);
  FilePagePool::SyncWritePage(lp);
  lp->SetReferred(false);
  LeafPage *root = (LeafPage *)indexTree->GetRootPage();
  root->SaveRecords(pageMap, false);
  FilePagePool::SyncWritePage(root);
  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;

  indexTree = new IndexTree();
  indexTree->LoadIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                           vctVal, fileId);
  lp = new LeafPage(indexTree, 1);
  indexTree->IncPages();
  FilePagePool::SyncReadPage(lp);
  lp->SetPageStatus(PageStatus::VALID);
  lp->InitParameters();
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

  delete lp;
  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(LeafPageSplit_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testLeafPageSplit" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = IndexPage::MAX_DATA_LENGTH_LEAF / 10;
  MTreeMap<uint64_t, CachePage *> pageMap;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, GetFileId(), IndexType::PRIMARY);

  HeadPage *hp = indexTree->GetHeadPage();
  LeafPage *lp = (LeafPage *)indexTree->GetRootPage();
  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i + ROW_COUNT;
    *((DataValueLong *)vctVal[0]) = i + ROW_COUNT + 100LL;
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal,
                                    hp->GetAndIncRecordStamp(), nullptr);
    lp->InsertRecord(rr);
  }

  bool b = lp->SplitPage(pageMap, UINT8_MAX);
  BOOST_TEST(b);

  BranchPage *root = (BranchPage *)lp->GetParentPage();
  BOOST_TEST(pageMap.size() == root->GetRecordNumber() + 1);

  int count = 0;
  int limitLen = lp->GetMaxDataLength() * IndexPage::LOAD_FACTOR / 100;
  int maxLen = lp->GetMaxDataLength();
  BranchRecord *pbr = nullptr;
  LeafPage *prevPage = nullptr;

  for (uint32_t i = 0; i < root->GetRecordNumber(); i++) {
    BranchRecord &br = root->GetRecord(i, false);
    LeafPage *page = (LeafPage *)br.GetChildPage();
    BOOST_TEST(page->GetParentPage() == root);

    if (pbr != nullptr) {
      BOOST_TEST(br.CompareKey(*pbr) > 0);
      BOOST_TEST(page->GetPrevPage() == prevPage);
      BOOST_TEST(page->GetPrevPageId() == prevPage->GetPageId());
      BOOST_TEST(page == prevPage->GetNextPage());
      BOOST_TEST(page->GetPageId() == prevPage->GetNextPageId());
    } else {
      BOOST_TEST(page->GetPrevPage() == nullptr);
      BOOST_TEST(page->GetPrevPageId() == PAGE_NULL_POINTER);
    }

    if (i < root->GetRecordNumber() - 1) {
      BOOST_TEST(page->GetCommitedDataLength() >= limitLen);
    } else {
      BOOST_TEST(page->GetNextPage() == nullptr);
      BOOST_TEST(page->GetNextPageId() == PAGE_NULL_POINTER);
    }

    BOOST_TEST(page->GetCommitedDataLength() <= maxLen);
    BOOST_TEST(page->GetPageId() == br.GetChildPageId());

    pbr = &br;
    prevPage = page;
    LeafRecord *pcbr = nullptr;

    for (uint32_t j = 0; j < page->GetRecordNumber(); j++) {
      LeafRecord &cbr = page->GetRecord(j);
      if (pcbr != nullptr) {
        BOOST_TEST(cbr.CompareKey(*pcbr) > 0);
      }

      pcbr = &cbr;
      count++;
    }

    BOOST_TEST(pbr->CompareKey(pcbr->GetKey()) == 0);
  }

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal,
                                    hp->GetAndIncRecordStamp(), nullptr);
    lp->InsertRecord(lr, i);
  }

  b = lp->SplitPage(pageMap, UINT8_MAX);
  BOOST_TEST(b);
  BOOST_TEST(pageMap.size() == root->GetRecordNumber() + 1);

  pbr = nullptr;
  count = 0;
  prevPage = nullptr;

  for (uint32_t i = 0; i < root->GetRecordNumber(); i++) {
    BranchRecord &br = root->GetRecord(i, false);
    LeafPage *page = (LeafPage *)br.GetChildPage();
    BOOST_TEST(page->GetParentPage() == root);

    if (pbr != nullptr) {
      BOOST_TEST(br.CompareKey(*pbr) > 0);
      BOOST_TEST(page->GetPrevPage() == prevPage);
      BOOST_TEST(page->GetPrevPageId() == prevPage->GetPageId());
      BOOST_TEST(page == prevPage->GetNextPage());
      BOOST_TEST(page->GetPageId() == prevPage->GetNextPageId());
    } else {
      BOOST_TEST(page->GetPrevPage() == nullptr);
      BOOST_TEST(page->GetPrevPageId() == PAGE_NULL_POINTER);
    }

    if (i == root->GetRecordNumber() - 1) {
      BOOST_TEST(page->GetNextPage() == nullptr);
      BOOST_TEST(page->GetNextPageId() == PAGE_NULL_POINTER);
    }

    BOOST_TEST(page->GetCommitedDataLength() <= maxLen);
    BOOST_TEST(page->GetPageId() == br.GetChildPageId());

    BOOST_TEST(page->IsBeginPage() == (i == 0));
    BOOST_TEST(page->IsEndPage() == (i == root->GetRecordNumber() - 1));

    pbr = &br;
    prevPage = page;
    LeafRecord *pcbr = nullptr;

    for (uint32_t j = 0; j < page->GetRecordNumber(); j++) {
      LeafRecord &cbr = page->GetRecord(j);
      if (pcbr != nullptr) {
        BOOST_TEST(cbr.CompareKey(*pcbr) > 0);
      }

      pcbr = &cbr;
      count++;
    }

    BOOST_TEST(pbr->CompareKey(pcbr->GetKey()) == 0);
    BOOST_TEST(page->IsBeginPage() == (i == 0));
    BOOST_TEST(page->IsEndPage() == (i == root->GetRecordNumber() - 1));
    page->SetReferred(false);
  }

  BOOST_TEST(count == 2 * ROW_COUNT);
  BOOST_TEST(root->IsBeginPage());
  BOOST_TEST(root->IsEndPage());
  root->SetReferred(false);

  indexTree->Close();
  CachePagePool::ClearPool();
  delete indexTree;
  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
