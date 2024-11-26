#include "../../src/core/BranchPage.h"
#include "../../src/core/BranchRecord.h"
#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/utils/BytesFuncs.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_AUTO_TEST_SUITE(CoreTest)
BOOST_AUTO_TEST_CASE(BranchPage_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testBranchPage" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 100;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);
  BranchPage *bp =
      (BranchPage *)indexTree.ApplyIndexPages(nullptr, (Byte)1, 1, false)[0];

  vctKey.push_back(new DataValueLong(1LL));
  vctVal.push_back(new DataValueLong(1LL));
  RawKey key(vctKey);
  int32_t pos = bp->SearchKey(key);

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr =
        new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
    BranchRecord *rr = new BranchRecord(IndexType::PRIMARY, lr, i + 100);
    bp->InsertRecord(rr, i);
    delete lr;
  }

  bool b = bp->SaveRecords();
  BOOST_TEST(b);

  *((DataValueLong *)vctKey[0]) = 0;
  *((DataValueLong *)vctVal[0]) = 100;
  LeafRecord *lr =
      new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
  BranchRecord *rr = new BranchRecord(IndexType::PRIMARY, lr, 100);
  BranchRecord &first = bp->GetRecord(0, true);
  BOOST_TEST(rr->CompareTo(first) == 0);
  delete lr;
  delete rr;

  *((DataValueLong *)vctKey[0]) = ROW_COUNT - 1;
  *((DataValueLong *)vctVal[0]) = ROW_COUNT + 99;
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
  rr = new BranchRecord(IndexType::PRIMARY, lr, ROW_COUNT + 99);
  BranchRecord &last = bp->GetRecord(ROW_COUNT - 1, false);
  BOOST_TEST(rr->CompareTo(last) == 0);
  delete lr;
  delete rr;

  *((DataValueLong *)vctKey[0]) = ROW_COUNT / 2;
  *((DataValueLong *)vctVal[0]) = ROW_COUNT / 2 + 100;
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
  rr = new BranchRecord(IndexType::PRIMARY, lr, ROW_COUNT / 2 + 100);
  BranchRecord &mid = bp->GetRecord(ROW_COUNT / 2, false);
  BOOST_TEST(rr->CompareTo(mid) == 0);
  delete lr;
  delete rr;

  bp->SetReferred(false);
  indexTree.Close();

  dvKey->DecRef();
  dvVal->DecRef();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchPageSave_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testBranchPageSave" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = IndexPage::MAX_DATA_LENGTH_BRANCH / 32;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);
  BranchPage *bp =
      (BranchPage *)indexTree.ApplyIndexPages(nullptr, (Byte)1, 1, false)[0];

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr =
        new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
    BranchRecord *rr = new BranchRecord(IndexType::PRIMARY, lr, i);
    bp->InsertRecord(rr, i);
    delete lr;
  }

  bool b = bp->SaveRecords();
  BOOST_TEST(b);

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr =
        new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
    BranchRecord *rr = new BranchRecord(IndexType::PRIMARY, lr, i);
    uint32_t index = bp->SearchRecord(*rr);
    BranchRecord &br = bp->GetRecord(index, false);
    BOOST_TEST(br.CompareTo(*rr) == 0);

    delete lr;
    delete rr;
  }

  bp->SetReferred(false);
  indexTree.Close();
  dvKey->DecRef();
  dvVal->DecRef();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchPageDelete_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testBranchPage" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = 100;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);
  BranchPage *bp =
      (BranchPage *)indexTree.ApplyIndexPages(nullptr, (Byte)1, 1, false)[0];

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr =
        new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
    BranchRecord *rr = new BranchRecord(IndexType::PRIMARY, lr, i + 100);
    bp->InsertRecord(rr, i);
    delete lr;
  }

  bool b = bp->SaveRecords();
  BOOST_TEST(b);

  for (int i = ROW_COUNT - 1; i >= 0; i--) {
    if (i % 2 == 1) {
      BranchRecord *br = bp->DeleteRecord(i);
      delete br;
    }
  }

  b = bp->SaveRecords();
  BOOST_TEST(b);
  BOOST_TEST(ROW_COUNT / 2 == bp->GetRecordNumber());

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    RawKey key(vctKey);

    int32_t pos = bp->SearchKey(key);
    BranchRecord &br = bp->GetRecord(pos, true);

    if (i == 99) {
      BOOST_TEST(pos == 49);
      BOOST_TEST(br.CompareKey(key) < 0);
    } else if (i % 2 == 1) {
      BOOST_TEST(pos - 1 == i / 2);
      BOOST_TEST(br.CompareKey(key) > 0);
    } else {
      BOOST_TEST(pos == i / 2);
      BOOST_TEST(br.CompareKey(key) == 0);
    }
  }

  bp->SetReferred(false);
  indexTree.Close();
  dvKey->DecRef();
  dvVal->DecRef();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchPageSearchKey_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testBranchPageSearchKey" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueVarChar *dvKey = new DataValueVarChar(1000);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, 2003, IndexType::PRIMARY);

  BranchPage *bp =
      (BranchPage *)indexTree.ApplyIndexPages(nullptr, (Byte)1, 1, false)[0];

  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));
  uint64_t arLong[] = {1, 123456, 3456, 789, 8776};
  for (int i = 0; i < 5; i++) {
    MString str = "testString" + ToMString(arLong[i]);
    *((DataValueVarChar *)vctKey[0]) = str.c_str();
    *((DataValueLong *)vctVal[0]) = i + 100;
    LeafRecord *lr =
        new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
    BranchRecord *rr = new BranchRecord(IndexType::PRIMARY, lr, i + 100);
    bp->InsertRecord(rr, i);
    delete lr;
  }

  uint64_t arKey[] = {0, 20, 135, 70, 999};
  int arPos[] = {0, 2, 2, 3, 4};
  for (int i = 0; i < 5; i++) {
    MString str = "testString" + ToMString(arKey[i]);
    *((DataValueVarChar *)vctKey[0]) = str.c_str();
    RawKey key(vctKey);
    BOOST_TEST(arPos[i] == bp->SearchKey(key));
  }

  bp->SetReferred(false);
  indexTree.Close();
  dvKey->DecRef();
  dvVal->DecRef();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchPageSplit_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testBranchPageSplit" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  const int ROW_COUNT = IndexPage::MAX_DATA_LENGTH_BRANCH / 10;
  MTreeMap<uint64_t, CachePage *> pageMap;

  DataValueLong *dvKey = new DataValueLong(100);
  DataValueLong *dvVal = new DataValueLong(200);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, GetFileId(), IndexType::PRIMARY);
  HeadPage *hp = indexTree->GetHeadPage();
  indexTree->GetRootPage()->SetReferred(false);

  MVector<IndexPage *> vctPage =
      indexTree->ApplyIndexPages(nullptr, 0, ROW_COUNT, false);
  BranchPage *bp =
      (BranchPage *)indexTree->ApplyIndexPages(nullptr, 1, 1, false)[0];
  bp->SetBeginPage(true);
  bp->SetEndPage(true);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i + ROW_COUNT;
    *((DataValueLong *)vctVal[0]) = i + ROW_COUNT + 100LL;
    LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal,
                                    hp->GetAndIncRecordStamp(), nullptr);
    LeafPage *lp = (LeafPage *)vctPage[i];
    lp->SetParentPage(bp);
    lp->SetParentPageID(bp->GetPageId());
    lp->InsertRecord(lr, 0);
    BranchRecord *br =
        new BranchRecord(IndexType::PRIMARY, lr, lp->GetPageId(), lp);
    bp->InsertRecord(br, i);
  }

  bool b = bp->SplitPage(pageMap, UINT8_MAX);
  BOOST_TEST(b);

  BranchPage *root = (BranchPage *)bp->GetParentPage();
  size_t mapSize =
      ROW_COUNT - bp->GetRecordNumber() + 1 + root->GetRecordNumber();
  size_t rootSize = root->GetRecordNumber();
  BOOST_TEST(pageMap.size() == mapSize);

  int count = 0;
  int limitLen = root->GetMaxDataLength() * IndexPage::LOAD_FACTOR / 100;
  int maxLen = root->GetMaxDataLength();
  BranchRecord *pbr = nullptr;

  for (uint32_t i = 0; i < root->GetRecordNumber(); i++) {
    BranchRecord &br = root->GetRecord(i, false);
    if (pbr != nullptr) {
      BOOST_TEST(br.CompareKey(*pbr) > 0);
    }

    BranchPage *page = (BranchPage *)br.GetChildPage();
    if (i < root->GetRecordNumber() - 1) {
      BOOST_TEST(page->GetCommitedDataLength() >= limitLen);
    }
    BOOST_TEST(page->GetCommitedDataLength() <= maxLen);
    BOOST_TEST(page->GetPageId() == br.GetChildPageId());
    BOOST_TEST(page->GetParentPage() == root);

    pbr = &br;
    BranchRecord *pcbr = nullptr;

    for (uint32_t j = 0; j < page->GetRecordNumber(); j++) {
      BranchRecord &cbr = page->GetRecord(j, false);
      if (pcbr != nullptr) {
        BOOST_TEST(cbr.CompareKey(*pcbr) > 0);
      }

      LeafPage *cpage = (LeafPage *)cbr.GetChildPage();
      BOOST_TEST(cpage->GetPageId() == cbr.GetChildPageId());
      BOOST_TEST(cpage->GetParentPage() == page);
      BOOST_TEST(cpage->GetParentPageId() == page->GetPageId());

      pcbr = &cbr;
      count++;
    }

    BOOST_TEST(pcbr->CompareKey(*pbr) == 0);
  }

  BOOST_TEST(count == ROW_COUNT);

  MVector<IndexPage *> vctPage2 =
      indexTree->ApplyIndexPages(nullptr, 0, ROW_COUNT, false);
  for (int i = 0; i < ROW_COUNT; i++) {
    *((DataValueLong *)vctKey[0]) = i;
    *((DataValueLong *)vctVal[0]) = i + 100LL;
    LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal,
                                    hp->GetAndIncRecordStamp(), nullptr);
    LeafPage *lp = (LeafPage *)vctPage2[i];
    lp->SetParentPage(bp);
    lp->SetParentPageID(bp->GetPageId());
    lp->InsertRecord(lr, 0);
    BranchRecord *br =
        new BranchRecord(IndexType::PRIMARY, lr, lp->GetPageId(), lp);
    bp->InsertRecord(br, i);
  }

  size_t bpSize = bp->GetRecordNumber();
  b = bp->SplitPage(pageMap, UINT8_MAX);
  BOOST_TEST(b);

  mapSize +=
      bpSize - bp->GetRecordNumber() + root->GetRecordNumber() - rootSize;
  BOOST_TEST(pageMap.size() == mapSize);

  pbr = nullptr;
  count = 0;

  for (uint32_t i = 0; i < root->GetRecordNumber(); i++) {
    BranchRecord &br = root->GetRecord(i, false);
    if (pbr != nullptr) {
      BOOST_TEST(br.CompareKey(*pbr) > 0);
    }

    BranchPage *page = (BranchPage *)br.GetChildPage();
    BOOST_TEST(page->GetCommitedDataLength() <= maxLen);
    BOOST_TEST(page->GetPageId() == br.GetChildPageId());
    BOOST_TEST(page->GetParentPage() == root);

    pbr = &br;
    BranchRecord *pcbr = nullptr;

    for (uint32_t j = 0; j < page->GetRecordNumber(); j++) {
      BranchRecord &cbr = page->GetRecord(j, false);
      if (pcbr != nullptr) {
        BOOST_TEST(cbr.CompareKey(*pcbr) > 0);
      }

      LeafPage *cpage = (LeafPage *)cbr.GetChildPage();
      BOOST_TEST(cpage->GetPageId() == cbr.GetChildPageId());
      BOOST_TEST(cpage->GetParentPage() == page);
      BOOST_TEST(cpage->GetParentPageId() == page->GetPageId());
      cpage->SetReferred(false);

      pcbr = &cbr;
      count++;
    }

    BOOST_TEST(pcbr->CompareKey(*pbr) == 0);
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
