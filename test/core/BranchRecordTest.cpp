#include "../../src/core/BranchRecord.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/pool/PageBufferPool.h"
#include "../../src/utils/BytesFuncs.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)
BOOST_AUTO_TEST_CASE(BranchRecord_PrimaryKey_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testBranchRecord_PrimaryKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong dvKey(100);
  DataValueLong dvVal(200);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                             vctVal, 0, IndexType::PRIMARY);
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
  lr->DecRef();
  delete br;
  delete br2;
  bp->DecRef();

  indexTree->Close();
}

BOOST_AUTO_TEST_CASE(BranchRecord_UniqueKey_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testBranchRecord_UniqueKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong dvKey(100);
  DataValueLong dvVal(200);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree *indexPri = new IndexTree();
  indexPri->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, 0, IndexType::PRIMARY);

  vctKey.push_back(dvKey.Clone(true));
  vctVal.push_back(dvVal.Clone(true));
  LeafRecord *lr = new LeafRecord(indexPri, vctKey, vctVal, 1, nullptr);

  DataValueFixChar dvFix("1234567890abcdefghijklmn", 26, 100);
  VectorDataValue vctSec = {dvFix.Clone(), dvKey.Clone()};
  IndexTree *indexSec = new IndexTree();
  indexSec->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, 1, IndexType::UNIQUE);
  vctSec = {dvFix.Clone(true), dvKey.Clone(true)};
  Byte *bys = lr->GetBysValue() + UI16_2_LEN;
  uint16_t lKey = lr->GetKeyLength();
  LeafRecord *lrSec =
      new LeafRecord(indexSec, vctSec, bys, lKey, ActionType::INSERT, nullptr);

  BranchRecord *br = new BranchRecord(indexSec, lrSec, 20);
  Byte buff[1000];
  br->SaveData(buff);

  BranchPage *bp = (BranchPage *)indexSec->AllocateNewPage(1, (Byte)1);
  BranchRecord *br2 = new BranchRecord(bp, buff);

  RawKey rkey(vctSec);
  BOOST_TEST(br2->CompareKey(rkey) == 0);

  BOOST_TEST(br2->GetChildPageId() == 20);
  BOOST_TEST(br2->GetValueLength() == 0);
  BOOST_TEST(br->IsSole());
  BOOST_TEST(!br2->IsSole());
  BOOST_TEST(br->EqualPageId(*br2));
  BOOST_TEST(br2->CompareKey(*br) == 0);
  BOOST_TEST(br2->CompareTo(*br) == 0);
  BOOST_TEST(br->GetParentPage() == nullptr);
  BOOST_TEST(br2->GetParentPage() == bp);
  BOOST_TEST(br2->GetBysValue() == buff);
  BOOST_TEST(br->GetTotalLength() == br2->GetTotalLength());

  lr->DecRef();
  lrSec->DecRef();
  delete br;
  delete br2;
  bp->DecRef();

  indexSec->Close();
  indexPri->Close();
}

BOOST_AUTO_TEST_CASE(BranchRecord_NonUniqueKey_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testBranchRecord_NonUniqueKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong dvKey(100);
  DataValueLong dvVal(200);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree *indexPri = new IndexTree();
  indexPri->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, 0, IndexType::PRIMARY);

  vctKey.push_back(dvKey.Clone(true));
  vctVal.push_back(dvVal.Clone(true));
  LeafRecord *lr = new LeafRecord(indexPri, vctKey, vctVal, 1, nullptr);

  DataValueFixChar dvFix("1234567890abcdefghijklmn", 26, 100);
  VectorDataValue vctSec = {dvFix.Clone(), dvKey.Clone()};
  IndexTree *indexSec = new IndexTree();
  indexSec->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, 1, IndexType::NON_UNIQUE);
  vctSec = {dvFix.Clone(true), dvKey.Clone(true)};
  Byte *bys = lr->GetBysValue() + UI16_2_LEN;
  uint16_t lKey = lr->GetKeyLength();
  LeafRecord *lrSec =
      new LeafRecord(indexSec, vctSec, bys, lKey, ActionType::INSERT, nullptr);

  BranchRecord *br = new BranchRecord(indexSec, lrSec, 20);
  Byte buff[1000];
  br->SaveData(buff);

  BranchPage *bp = (BranchPage *)indexSec->AllocateNewPage(1, (Byte)1);
  BranchRecord *br2 = new BranchRecord(bp, buff);

  RawKey rkey(vctSec);
  BOOST_TEST(br2->CompareKey(rkey) == 0);

  BOOST_TEST(br2->GetChildPageId() == 20);
  BOOST_TEST(br2->GetValueLength() == 8);
  BOOST_TEST(br->IsSole());
  BOOST_TEST(!br2->IsSole());
  BOOST_TEST(br->EqualPageId(*br2));
  BOOST_TEST(br2->CompareKey(*br) == 0);
  BOOST_TEST(br2->CompareTo(*br) == 0);
  BOOST_TEST(br->GetParentPage() == nullptr);
  BOOST_TEST(br2->GetParentPage() == bp);
  BOOST_TEST(br2->GetBysValue() == buff);
  BOOST_TEST(br->GetTotalLength() == br2->GetTotalLength());

  lr->DecRef();
  lrSec->DecRef();
  delete br;
  delete br2;
  bp->DecRef();

  indexSec->Close();
  indexPri->Close();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
