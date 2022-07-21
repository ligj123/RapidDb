#include "../../src/core/BranchRecord.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/utils/Utilitys.h"
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)
BOOST_AUTO_TEST_CASE(BranchRecord_test) {
  const string FILE_NAME = "./dbTest/testBranchRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong dvKey(100, true);
  DataValueLong dvVal(200, false);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree *indexTree =
      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal, IndexType::PRIMARY);
  BranchPage *bp = (BranchPage *)indexTree->AllocateNewPage(1, (Byte)1);

  vctKey.push_back(dvKey.Clone(true));
  vctVal.push_back(dvVal.Clone(true));
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  BranchRecord *br = new BranchRecord(indexTree, lr, 20);

  Byte buff[100];
  br->SaveData(buff);
  BranchRecord *br2 = new BranchRecord(bp, buff);

  RawKey *key = br2->GetKey();
  BOOST_TEST(lr->CompareKey(*key) == 0);
  VectorDataValue vct;
  br2->GetListKey(vct);
  BOOST_TEST(vct.size() == 1);
  BOOST_TEST(dvKey == *vct[0]);

  BOOST_TEST(br2->GetChildPageId() == 20);
  BOOST_TEST(br2->GetValueLength() == 0);
  BOOST_TEST(br->IsSole());
  BOOST_TEST(!br2->IsSole());
  BOOST_TEST(br->EqualPageId(*br2));
  BOOST_TEST(br2->CompareKey(*lr) == 0);
  BOOST_TEST(br2->CompareKey(*key) == 0);
  BOOST_TEST(br2->CompareTo(*br) == 0);
  BOOST_TEST(br->GetParentPage() == nullptr);
  BOOST_TEST(br2->GetParentPage() == bp);
  BOOST_TEST(br2->GetBysValue() == buff);
  BOOST_TEST(br->GetTotalLength() == br2->GetTotalLength());

  delete key;
  lr->ReleaseRecord();
  br->ReleaseRecord();
  br2->ReleaseRecord();
  bp->DecRef();

  indexTree->Close();
}
//
// BOOST_AUTO_TEST_CASE(BranchRecord_Equal_test) {
//  const string FILE_NAME =
//      "./dbTest/testBranchRecordEqual" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//
//  DataValueLong *dvKey = new DataValueLong(100LL, true);
//  DataValueLong *dvVal = new DataValueLong(200LL, false);
//  VectorDataValue vctKey = {dvKey->Clone()};
//  VectorDataValue vctVal = {dvVal->Clone()};
//
//  IndexTree *indexTree =
//      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal,
//      IndexType::PRIMARY);
//  BranchPage *bp =
//      (BranchPage *)indexTree->AllocateNewPage(UINT32_MAX, (Byte)1);
//
//  Byte buff1[100];
//  uint16_t pos = 0;
//  uint16_t len =
//      (uint16_t)(sizeof(uint16_t) * 2 + dvKey->GetPersistenceLength(true) +
//                 BranchRecord::PAGE_ID_LEN);
//  UInt16ToBytes(len, buff1 + pos);
//  pos += 2;
//  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff1 + pos);
//  pos += 2;
//
//  pos += dvKey->WriteData(buff1 + pos, true);
//  UInt64ToBytes(200, buff1 + pos);
//  pos += sizeof(uint64_t);
//
//  Byte buff2[100];
//  pos = 0;
//  UInt16ToBytes(len, buff2 + pos);
//  pos += 2;
//  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
//  pos += 2;
//
//  pos += dvKey->WriteData(buff2 + pos, true);
//  UInt64ToBytes(200, buff2 + pos);
//  pos += sizeof(uint64_t);
//
//  BranchRecord *rr1 = new BranchRecord(bp, buff1);
//  BranchRecord *rr2 = new BranchRecord(bp, buff2);
//
//  BOOST_TEST(rr1->CompareKey(*rr2) == 0);
//  RawKey *key = rr2->GetKey();
//  BOOST_TEST(rr1->CompareKey(*key) == 0);
//  delete key;
//  BOOST_TEST(0 == rr1->CompareTo(*rr2));
//
//  pos = 0;
//  UInt16ToBytes(len, buff2 + pos);
//  pos += 2;
//  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
//  pos += 2;
//
//  pos += dvKey->WriteData(buff2 + pos, true);
//  UInt64ToBytes(210, buff2 + pos);
//  pos += sizeof(uint64_t);
//
//  BranchRecord *rr3 = new BranchRecord(bp, buff2);
//
//  BOOST_TEST(rr1->CompareKey(*rr3) == 0);
//  RawKey *key2 = rr3->GetKey();
//  BOOST_TEST(rr1->CompareKey(*key2) == 0);
//  delete key2;
//  BOOST_TEST(0 == rr1->CompareTo(*rr3));
//
//  delete dvKey;
//  dvKey = new DataValueLong(110, true);
//  pos = 0;
//  UInt16ToBytes(len, buff2 + pos);
//  pos += 2;
//  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
//  pos += 2;
//
//  pos += dvKey->WriteData(buff2 + pos, true);
//  UInt64ToBytes(200, buff2 + pos);
//  pos += sizeof(uint64_t);
//
//  BranchRecord *rr4 = new BranchRecord(bp, buff2);
//
//  BOOST_TEST(rr1->CompareTo(*rr4) < 0);
//  BOOST_TEST(rr1->CompareKey(*rr4) < 0);
//  RawKey *key3 = rr4->GetKey();
//  BOOST_TEST(0 > rr1->CompareKey(*key3));
//  delete key3;
//
//  rr1->ReleaseRecord();
//  rr2->ReleaseRecord();
//  rr3->ReleaseRecord();
//  rr4->ReleaseRecord();
//  delete dvKey;
//  delete dvVal;
//
//  bp->DecRef();
//  indexTree->Close();
//  PageBufferPool::ClearPool();
//
//  std::filesystem::remove(std::filesystem::path(FILE_NAME));
//}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
