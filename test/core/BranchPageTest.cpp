#include "../../src/core/BranchPage.h"
#include "../../src/core/BranchRecord.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/utils/BytesFuncs.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_AUTO_TEST_SUITE(CoreTest)
BOOST_AUTO_TEST_CASE(BranchPage_test) {
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
  bool bFind;
  int32_t pos = bp->SearchKey(key, bFind);
  BOOST_TEST(!bFind);

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
    bool bFind;
    uint32_t index = bp->SearchRecord(*rr, bFind);
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
    bool bFind;

    int32_t pos = bp->SearchKey(key, bFind);
    if (i % 2 == 1) {
      BOOST_TEST(!bFind);
    } else {
      BOOST_TEST(bFind);
      BOOST_TEST(pos == i / 2);
    }
  }

  bp->SetReferred(false);
  indexTree.Close();
  dvKey->DecRef();
  dvVal->DecRef();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchPageSearchKey_test) {
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
  int arPos[] = {0, 2, 2, 3, 5};
  for (int i = 0; i < 5; i++) {
    MString str = "testString" + ToMString(arKey[i]);
    *((DataValueVarChar *)vctKey[0]) = str.c_str();
    RawKey key(vctKey);
    bool bFind;
    BOOST_TEST(arPos[i] == bp->SearchKey(key, bFind));
  }

  bp->SetReferred(false);
  indexTree.Close();
  dvKey->DecRef();
  dvVal->DecRef();
  CachePagePool::ClearPool();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
