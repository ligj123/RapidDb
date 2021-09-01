#include "../../src/core/BranchRecord.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/utils/Utilitys.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_AUTO_TEST_SUITE(CoreTest)

BOOST_AUTO_TEST_CASE(BranchRecord_test) {
  const string FILE_NAME =
      "./dbTest/testBranchRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100, true);
  DataValueLong *dvVal = new DataValueLong(200, false);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
  BranchPage *bp =
      (BranchPage *)indexTree->AllocateNewPage(UINT64_MAX, (Byte)1);

  Byte buff[100];
  uint16_t pos = 0;
  uint16_t len =
      (uint16_t)(sizeof(uint16_t) * 2 + dvKey->GetPersistenceLength(true) +
                 BranchRecord::PAGE_ID_LEN);
  UInt16ToBytes(len, buff + pos);
  pos += 2;
  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff + pos);
  pos += 2;

  pos += dvKey->WriteData(buff + pos, true);
  UInt64ToBytes(200, buff + pos);
  pos += sizeof(uint64_t);

  BranchRecord *rr = new BranchRecord(bp, buff);
  BOOST_TEST(buff == rr->GetBysValue());
  BOOST_TEST(8, rr->GetKeyLength());
  BOOST_TEST(0 == rr->GetValueLength());
  BOOST_TEST(20 == rr->GetTotalLength());

  VectorDataValue vctKey2;
  rr->GetListKey(vctKey2);
  BOOST_TEST(*dvKey == *vctKey2[0]);
  BOOST_TEST(200 == rr->GetChildPageId());

  BranchRecord *rr2 = new BranchRecord(indexTree, rr, 300);
  BOOST_TEST(8 == rr2->GetKeyLength());
  BOOST_TEST(0 == rr2->GetValueLength());
  BOOST_TEST(20 == rr2->GetTotalLength());

  vctKey.clear();
  rr2->GetListKey(vctKey);
  BOOST_TEST(*dvKey == *vctKey[0]);
  BOOST_TEST(300 == rr2->GetChildPageId());

  delete dvKey;
  delete dvVal;
  rr->ReleaseRecord();
  rr->ReleaseRecord();
  bp->DecRefCount();
  indexTree->Close(true);
  PageBufferPool::ClearPool();
  std::filesystem::remove(std::filesystem::path(FILE_NAME));
}

BOOST_AUTO_TEST_CASE(BranchRecord_Equal_test) {
  const string FILE_NAME =
      "./dbTest/testBranchRecordEqual" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100LL, true);
  DataValueLong *dvVal = new DataValueLong(200LL, false);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};

  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
  BranchPage *bp =
      (BranchPage *)indexTree->AllocateNewPage(UINT64_MAX, (Byte)1);

  Byte buff1[100];
  uint16_t pos = 0;
  uint16_t len =
      (uint16_t)(sizeof(uint16_t) * 2 + dvKey->GetPersistenceLength(true) +
                 BranchRecord::PAGE_ID_LEN);
  UInt16ToBytes(len, buff1 + pos);
  pos += 2;
  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff1 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff1 + pos, true);
  UInt64ToBytes(200, buff1 + pos);
  pos += sizeof(uint64_t);

  Byte buff2[100];
  pos = 0;
  UInt16ToBytes(len, buff2 + pos);
  pos += 2;
  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff2 + pos, true);
  UInt64ToBytes(200, buff2 + pos);
  pos += sizeof(uint64_t);

  BranchRecord *rr1 = new BranchRecord(bp, buff1);
  BranchRecord *rr2 = new BranchRecord(bp, buff2);

  BOOST_TEST(rr1->CompareKey(*rr2) == 0);
  RawKey *key = rr2->GetKey();
  BOOST_TEST(rr1->CompareKey(*key) == 0);
  delete key;
  BOOST_TEST(0 == rr1->CompareTo(*rr2));

  pos = 0;
  UInt16ToBytes(len, buff2 + pos);
  pos += 2;
  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff2 + pos, true);
  UInt64ToBytes(210, buff2 + pos);
  pos += sizeof(uint64_t);

  BranchRecord *rr3 = new BranchRecord(bp, buff2);

  BOOST_TEST(rr1->CompareKey(*rr3) == 0);
  RawKey *key2 = rr3->GetKey();
  BOOST_TEST(rr1->CompareKey(*key2) == 0);
  delete key2;
  BOOST_TEST(0 == rr1->CompareTo(*rr3));

  delete dvKey;
  dvKey = new DataValueLong(110, true);
  pos = 0;
  UInt16ToBytes(len, buff2 + pos);
  pos += 2;
  UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff2 + pos, true);
  UInt64ToBytes(200, buff2 + pos);
  pos += sizeof(uint64_t);

  BranchRecord *rr4 = new BranchRecord(bp, buff2);

  BOOST_TEST(rr1->CompareTo(*rr4) < 0);
  BOOST_TEST(rr1->CompareKey(*rr4) < 0);
  RawKey *key3 = rr4->GetKey();
  BOOST_TEST(0 > rr1->CompareKey(*key3));
  delete key3;

  rr1->ReleaseRecord();
  rr2->ReleaseRecord();
  rr3->ReleaseRecord();
  rr4->ReleaseRecord();
  delete dvKey;
  delete dvVal;

  bp->DecRefCount();
  indexTree->Close(true);
  PageBufferPool::ClearPool();

  std::filesystem::remove(std::filesystem::path(FILE_NAME));
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
