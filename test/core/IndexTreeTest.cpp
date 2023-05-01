#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/pool/PageDividePool.h"
#include "../../src/pool/StoragePool.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/utils/Utilitys.h"
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)
//
// BOOST_AUTO_TEST_CASE(IndexTreeInsertRecord_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexTreeInsertRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 1000;
//
//  DataValueLong *dvKey = new DataValueLong(100);
//  DataValueLong *dvVal = new DataValueLong(200);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree();
//  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 0,
//                         IndexType::PRIMARY);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    *((DataValueLong *)vctVal[0]) = i + 100LL;
//    LeafRecord *rr =
//        new LeafRecord(indexTree, vctKey, vctVal,
//                       indexTree->GetHeadPage()->ReadRecordStamp(), nullptr);
//    IndexPage *idxPage = nullptr;
//    indexTree->SearchRecursively(*rr, true, idxPage, true);
//    assert(idxPage != nullptr && idxPage->GetPageType() ==
//    PageType::LEAF_PAGE);
//
//    ((LeafPage *)idxPage)->InsertRecord(rr, false);
//  }
//
//  IndexTree::TestCloseWait(indexTree);
//
//  indexTree = new IndexTree();
//  indexTree->InitIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 0);
//  LeafPage *lp = indexTree->GetBeginPage();
//  uint64_t idx = 0;
//
//  while (lp != nullptr) {
//    for (uint32_t i = 0; i < lp->GetRecordNumber(); i++) {
//      VectorDataValue v1, v2;
//      LeafRecord *lr = lp->GetRecord(i);
//      *((DataValueLong *)vctKey[0]) = idx;
//      RawKey key(vctKey);
//      BOOST_TEST(lr->CompareKey(key) == 0);
//      lr->GetListValue(v2);
//      BOOST_TEST(v2[0]->GetLong() == (idx + 100));
//    }
//  }
//
//  IndexTree::TestCloseWait(indexTree);
//  delete dvKey;
//  delete dvVal;
//}

// BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToNonUniqueIndex_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexRepeatedKey" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 30000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  Byte bys[100];
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
//    Int64ToBytes(i + 100, bys, true);
//    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  VectorLeafRecord vct;
//  indexTree->QueryRecord(nullptr, nullptr, false, true, vct);
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    VectorDataValue v1, v2;
//    vct[i]->GetListKey(v1);
//    vct[i]->GetListValue(v2);
//    int64_t key = *(DataValueLong *)v1[0];
//    int64_t val = *(DataValueLong *)v2[0];
//    BOOST_TEST((val - 100) % (ROW_COUNT / 3) == key);
//    BOOST_TEST(key == i / 3);
//  }
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedKeyToUniqueIndex_test) {
//  const string FILE_NAME = ROOT_PATH +
//      "/testIndexRepeatedKey" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 30000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//
//  vctKey.push_back(new DataValueLong(10, true));
//  vctVal.push_back(new DataValueLong(100, true));
//
//  LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, 0);
//  ErrorMsg *err = indexTree->InsertRecord(rr);
//  BOOST_TEST(err == nullptr);
//
//  *((DataValueLong *)vctVal[0]) = 200;
//  rr = new LeafRecord(indexTree, vctKey, vctVal, 1);
//  err = indexTree->InsertRecord(rr);
//  BOOST_TEST(err->getErrId() == CORE_REPEATED_RECORD);
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeInsertRepeatedRecordToNonUniqueIndex_test) {
//  const string FILE_NAME = ROOT_PATH +
//      "/testIndexRepeatedRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  Byte bys[100];
//
//  *((DataValueLong *)vctKey[0]) = 10;
//  Int64ToBytes(100, bys, true);
//  LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
//  ErrorMsg *err = indexTree->InsertRecord(rr);
//  BOOST_TEST(err == nullptr);
//
//  LeafRecord *rr2 = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
//  err = indexTree->InsertRecord(rr2);
//  BOOST_TEST(err->getErrId() == CORE_REPEATED_RECORD);
//
//  indexTree->Close(true);
//
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeGetRecordWithUniqueIndex_test) {
//  const string FILE_NAME = ROOT_PATH +
//      "/testIndexRepeatedRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 2000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, false);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    *((DataValueLong *)vctVal[0]) = i + 100LL;
//    LeafRecord *rr = new LeafRecord(
//        indexTree, vctKey, vctVal,
//        indexTree->GetHeadPage()->ReadRecordStamp());
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  vctKey.push_back(dvKey->Clone());
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    RawKey key(vctKey);
//    LeafRecord *rr = indexTree->GetRecord(key);
//    VectorDataValue vct;
//    rr->GetListValue(vct);
//    BOOST_TEST((i + 100) == (int64_t)(*(DataValueLong *)vct[0]));
//  }
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeGetRecordWithNonUniqueIndex_test) {
//  const string FILE_NAME = ROOT_PATH + "/testIndexGetRecord" + StrMSTime() +
//  ".dat"; const string TABLE_NAME = "testTable"; const int ROW_COUNT = 6000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  Byte bys[100];
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
//    Int64ToBytes(100 + i, bys, true);
//    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  vctKey.push_back(dvKey->Clone());
//
//  for (int i = 0; i < ROW_COUNT / 3; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    RawKey key(vctKey);
//    VectorLeafRecord vlr;
//    indexTree->GetRecords(key, vlr);
//    assert(3 == vlr.size());
//    for (int j = 0; j < 3; j++) {
//      VectorDataValue vct;
//      vlr[j]->GetListValue(vct);
//      assert((i + 100 + ROW_COUNT / 3 * j) ==
//             (int64_t)(*(DataValueLong *)vct[0]));
//    }
//  }
//
//  PageDividePool::SetThreadStatus(false);
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeQueryRecordWithUniqueIndex_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexRepeatedRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 1000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    *((DataValueLong *)vctVal[0]) = i + 100LL;
//    LeafRecord *rr = new LeafRecord(
//        indexTree, vctKey, vctVal,
//        indexTree->GetHeadPage()->ReadRecordStamp());
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  *((DataValueLong *)vctKey[0]) = ROW_COUNT / 4;
//  RawKey keyStart(vctKey);
//  *((DataValueLong *)vctKey[0]) = ROW_COUNT / 2;
//  RawKey keyEnd(vctKey);
//
//  VectorLeafRecord vlr;
//  indexTree->QueryRecord(&keyStart, &keyEnd, true, true, vlr);
//  BOOST_TEST((ROW_COUNT / 4 + 1) == vlr.size());
//  for (int i = 0; i < vlr.size(); i++) {
//    VectorDataValue vct;
//    vlr[i]->GetListKey(vct);
//    BOOST_TEST((i + ROW_COUNT / 4) == (int64_t)(*(DataValueLong *)vct[0]));
//  }
//  vlr.RemoveAll();
//
//  indexTree->QueryRecord(&keyStart, &keyEnd, false, true, vlr);
//  BOOST_TEST((ROW_COUNT / 4) == vlr.size());
//  for (int i = 0; i < vlr.size(); i++) {
//    VectorDataValue vct;
//    vlr[i]->GetListKey(vct);
//    BOOST_TEST((i + ROW_COUNT / 4 + 1) == (int64_t)(*(DataValueLong
//    *)vct[0]));
//  }
//  vlr.RemoveAll();
//
//  indexTree->QueryRecord(&keyStart, &keyEnd, true, false, vlr);
//  BOOST_TEST((ROW_COUNT / 4) == vlr.size());
//  for (int i = 0; i < vlr.size(); i++) {
//    VectorDataValue vct;
//    vlr[i]->GetListKey(vct);
//    BOOST_TEST((i + ROW_COUNT / 4) == (int64_t)(*(DataValueLong *)vct[0]));
//  }
//  vlr.RemoveAll();
//
//  indexTree->QueryRecord(&keyStart, &keyEnd, false, false, vlr);
//  BOOST_TEST((ROW_COUNT / 4 - 1) == vlr.size());
//  for (int i = 0; i < vlr.size(); i++) {
//    VectorDataValue vct;
//    vlr[i]->GetListKey(vct);
//    BOOST_TEST((i + ROW_COUNT / 4 + 1) == (int64_t)(*(DataValueLong
//    *)vct[0]));
//  }
//  vlr.RemoveAll();
//
//  indexTree->QueryRecord(nullptr, &keyEnd, false, true, vlr);
//  BOOST_TEST((ROW_COUNT / 2 + 1) == vlr.size());
//  for (int i = 0; i < vlr.size(); i++) {
//    VectorDataValue vct;
//    vlr[i]->GetListKey(vct);
//    BOOST_TEST(i == (int64_t)(*(DataValueLong *)vct[0]));
//  }
//  vlr.RemoveAll();
//
//  indexTree->QueryRecord(&keyStart, nullptr, true, true, vlr);
//  BOOST_TEST((ROW_COUNT * 3 / 4) == vlr.size());
//  for (int i = 0; i < vlr.size(); i++) {
//    VectorDataValue vct;
//    vlr[i]->GetListKey(vct);
//    BOOST_TEST((i + ROW_COUNT / 4) == (int64_t)(*(DataValueLong *)vct[0]));
//  }
//
//  vlr.RemoveAll();
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeReadRecord_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexReadRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 6000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, false);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    *((DataValueLong *)vctVal[0]) = i + 100;
//    LeafRecord *rr = new LeafRecord(
//        indexTree, vctKey, vctVal,
//        indexTree->GetHeadPage()->ReadRecordStamp());
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  vctKey.push_back(dvKey->Clone());
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    RawKey key(vctKey);
//    VectorDataValue vctDv;
//    indexTree->ReadRecord(key, UINT64_MAX, vctDv);
//    BOOST_TEST((100 + i) == (int64_t)(*(DataValueLong *)vctDv[0]));
//  }
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeReadPrimaryKeysNoUnique_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexReadPrimaryKeys" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 6000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  Byte bys[100];
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i % (ROW_COUNT / 3);
//    Int64ToBytes(100 + i, bys, true);
//    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  vctKey.push_back(dvKey->Clone());
//
//  for (int i = 0; i < ROW_COUNT / 3; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    RawKey key(vctKey);
//    VectorRawKey vctRaw;
//    indexTree->ReadPrimaryKeys(key, vctRaw);
//    BOOST_TEST(3 == vctRaw.size());
//    for (int j = 0; j < 3; j++) {
//      Int64ToBytes((i + 100 + ROW_COUNT / 3 * j), bys, true);
//      BOOST_TEST(BytesCompare(bys, 8, vctRaw[j]->GetBysVal(),
//                              vctRaw[j]->GetLength()) == 0);
//    }
//  }
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeReadPrimaryKeysUnique_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexReadPrimaryKeys" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 6000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, true);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//  Byte bys[100];
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    Int64ToBytes(100 + i, bys, true);
//    LeafRecord *rr = new LeafRecord(indexTree, vctKey, bys, sizeof(int64_t));
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  vctKey.push_back(dvKey->Clone());
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    RawKey key(vctKey);
//    VectorRawKey vctRaw;
//    indexTree->ReadPrimaryKeys(key, vctRaw);
//    BOOST_TEST(1 == vctRaw.size());
//    Int64ToBytes((i + 100), bys, true);
//    assert(BytesCompare(bys, 8, vctRaw[0]->GetBysVal(),
//                        vctRaw[0]->GetLength()) == 0);
//  }
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeQueryIndexPaimary_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexQueryIndex" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 6000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, false);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    *((DataValueLong *)vctVal[0]) = i + 100;
//    LeafRecord *rr = new LeafRecord(
//        indexTree, vctKey, vctVal,
//        indexTree->GetHeadPage()->ReadRecordStamp());
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//
//  VectorRow vctRow;
//  indexTree->QueryIndex(nullptr, nullptr, false, false, vctRow);
//  BOOST_TEST(ROW_COUNT == vctRow.size());
//
//  Byte buff[100];
//  memset(buff, 0, 100);
//  RawKey key(buff, 8, false);
//  indexTree->QueryIndex(&key, nullptr, true, false, vctRow);
//  BOOST_TEST(ROW_COUNT == vctRow.size());
//  BOOST_TEST(100 == (int64_t)(*(DataValueLong *)(*vctRow[0])[0]));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             (int64_t)(*(DataValueLong *)(*vctRow[ROW_COUNT - 1])[0]));
//
//  Int64ToBytes(ROW_COUNT / 2, buff, true);
//  indexTree->QueryIndex(&key, nullptr, false, false, vctRow);
//  BOOST_TEST((ROW_COUNT / 2 - 1) == vctRow.size());
//  BOOST_TEST((ROW_COUNT / 2 + 101) ==
//             (int64_t)(*(DataValueLong *)(*vctRow[0])[0]));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             (int64_t)(*(DataValueLong *)(*vctRow[ROW_COUNT / 2 - 2])[0]));
//
//  indexTree->QueryIndex(nullptr, &key, false, false, vctRow);
//  BOOST_TEST((ROW_COUNT / 2) == vctRow.size());
//  BOOST_TEST(100 == (int64_t)(*(DataValueLong *)(*vctRow[0])[0]));
//  BOOST_TEST((ROW_COUNT / 2 + 99) ==
//             (int64_t)(*(DataValueLong *)(*vctRow[ROW_COUNT / 2 - 1])[0]));
//
//  indexTree->QueryIndex(nullptr, &key, false, true, vctRow);
//  BOOST_TEST((ROW_COUNT / 2 + 1) == vctRow.size());
//  BOOST_TEST(100 == (int64_t)(*(DataValueLong *)(*vctRow[0])[0]));
//  BOOST_TEST((ROW_COUNT / 2 + 100) ==
//             (int64_t)(*(DataValueLong *)(*vctRow[ROW_COUNT / 2])[0]));
//
//  Int64ToBytes(ROW_COUNT, buff, true);
//  indexTree->QueryIndex(nullptr, &key, false, false, vctRow);
//  BOOST_TEST((ROW_COUNT) == vctRow.size());
//  BOOST_TEST(100 == (int64_t)(*(DataValueLong *)(*vctRow[0])[0]));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             (int64_t)(*(DataValueLong *)(*vctRow[ROW_COUNT - 1])[0]));
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeQueryIndexUnique_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexQueryIndex" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 6000;
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, false);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//
//  Byte buff[100];
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i;
//    Int64ToBytes(i + 100, buff, true);
//    LeafRecord *rr = new LeafRecord(indexTree, vctKey, buff, 8);
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//
//  VectorRawKey vctRaw;
//  indexTree->QueryIndex(nullptr, nullptr, false, false, vctRaw);
//  BOOST_TEST(ROW_COUNT == vctRaw.size());
//
//  memset(buff, 0, 100);
//  RawKey key(buff, 8, false);
//  indexTree->QueryIndex(&key, nullptr, true, false, vctRaw);
//  BOOST_TEST(ROW_COUNT == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT - 1]->GetBysVal(), true));
//
//  Int64ToBytes(ROW_COUNT / 2, buff, true);
//  indexTree->QueryIndex(&key, nullptr, false, false, vctRaw);
//  BOOST_TEST((ROW_COUNT / 2 - 1) == vctRaw.size());
//  BOOST_TEST((ROW_COUNT / 2 + 101) ==
//             Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT / 2 - 2]->GetBysVal(), true));
//
//  indexTree->QueryIndex(nullptr, &key, false, false, vctRaw);
//  BOOST_TEST((ROW_COUNT / 2) == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT / 2 + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT / 2 - 1]->GetBysVal(), true));
//
//  indexTree->QueryIndex(nullptr, &key, false, true, vctRaw);
//  BOOST_TEST((ROW_COUNT / 2 + 1) == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT / 2 + 100) ==
//             Int64FromBytes(vctRaw[ROW_COUNT / 2]->GetBysVal(), true));
//
//  Int64ToBytes(ROW_COUNT, buff, true);
//  indexTree->QueryIndex(nullptr, &key, false, false, vctRaw);
//  BOOST_TEST((ROW_COUNT) == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT - 1]->GetBysVal(), true));
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
//
// BOOST_AUTO_TEST_CASE(IndexTreeQueryIndexNonUnique_test) {
//  const string FILE_NAME =ROOT_PATH +
//      "/testIndexQueryIndex" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  const int ROW_COUNT = 6000;
//  const int KEY_COUNT = 2000;
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal = new DataValueLong(200, false);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);
//
//  vctKey.push_back(dvKey->Clone());
//  vctVal.push_back(dvVal->Clone());
//
//  Byte buff[100];
//  for (int i = 0; i < ROW_COUNT; i++) {
//    *((DataValueLong *)vctKey[0]) = i % KEY_COUNT;
//    Int64ToBytes(i + 100, buff, true);
//    LeafRecord *rr = new LeafRecord(indexTree, vctKey, buff, 8);
//    indexTree->InsertRecord(rr);
//  }
//
//  indexTree->Close(true);
//
//  indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//
//  VectorRawKey vctRaw;
//  indexTree->QueryIndex(nullptr, nullptr, false, false, vctRaw);
//  BOOST_TEST(ROW_COUNT == vctRaw.size());
//
//  memset(buff, 0, 100);
//  RawKey key(buff, 8, false);
//  indexTree->QueryIndex(&key, nullptr, true, false, vctRaw);
//  BOOST_TEST(ROW_COUNT == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT - 1]->GetBysVal(), true));
//
//  Int64ToBytes(KEY_COUNT / 2, buff, true);
//  indexTree->QueryIndex(&key, nullptr, false, false, vctRaw);
//  BOOST_TEST((ROW_COUNT / 2 - 3) == vctRaw.size());
//  BOOST_TEST((KEY_COUNT / 2 + 101) ==
//             Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT / 2 - 4]->GetBysVal(), true));
//
//  indexTree->QueryIndex(nullptr, &key, false, false, vctRaw);
//  BOOST_TEST((ROW_COUNT / 2) == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST(5099 ==
//             Int64FromBytes(vctRaw[ROW_COUNT / 2 - 1]->GetBysVal(), true));
//
//  indexTree->QueryIndex(nullptr, &key, false, true, vctRaw);
//  BOOST_TEST((ROW_COUNT / 2 + 3) == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST(5100 ==
//             Int64FromBytes(vctRaw[ROW_COUNT / 2 + 2]->GetBysVal(), true));
//
//  Int64ToBytes(ROW_COUNT, buff, true);
//  indexTree->QueryIndex(nullptr, &key, false, false, vctRaw);
//  BOOST_TEST((ROW_COUNT) == vctRaw.size());
//  BOOST_TEST(100 == Int64FromBytes(vctRaw[0]->GetBysVal(), true));
//  BOOST_TEST((ROW_COUNT + 99) ==
//             Int64FromBytes(vctRaw[ROW_COUNT - 1]->GetBysVal(), true));
//
//  indexTree->Close(true);
//  delete dvKey;
//  delete dvVal;
//  PageBufferPool::ClearPool();
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
