#include "../../src/core/BranchRecord.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/IndexTree.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/statement/Statement.h"
#include "../../src/utils/BytesFuncs.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
class StatementEx : public Statement {
public:
  using Statement::Statement;
  ExprType GetActionType() override { return ExprType::EXPR_INSERT; }
  bool Exec() override { return false; }
  bool IsReadonly() override { return true; }
  MVector<uint16_t> SpliteRange(const MVector<LeafRecord> &vctBorder) override {
    return MVector<uint16_t>();
  }
};

BOOST_AUTO_TEST_SUITE(CoreTest)
BOOST_AUTO_TEST_CASE(BranchRecord_PrimaryKey_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testBranchRecord_PrimaryKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong dvKey(100);
  DataValueLong dvVal(200);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);
  BranchPage *bp =
      (BranchPage *)indexTree.ApplyIndexPages(nullptr, (Byte)1, 1, false)[0];

  StatementEx stmt(1, 1);
  vctKey.push_back(dvKey.Clone(true));
  vctVal.push_back(dvVal.Clone(true));
  LeafRecord *lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, &stmt, false);
  BranchRecord *br =
      new BranchRecord(indexTree.GetHeadPage()->GetIndexType(), lr, 20, bp);

  Byte buff[100];
  br->SaveData(buff);
  BranchRecord *br2 =
      new BranchRecord(indexTree.GetHeadPage()->GetIndexType(), buff, bp);
  RawKey key = lr->GetKey();

  BOOST_TEST(br2->GetChildPageId() == 20);
  BOOST_TEST(br2->GetValueLength() == 0);
  BOOST_TEST(br->IsSole());
  BOOST_TEST(!br2->IsSole());
  BOOST_TEST(br->EqualPageId(*br2));
  BOOST_TEST(br2->CompareKey(*lr) == 0);
  BOOST_TEST(br2->CompareKey(key) == 0);
  BOOST_TEST(br2->CompareTo(*br) == 0);
  BOOST_TEST(br->GetIndexType() == IndexType::PRIMARY);
  BOOST_TEST(br2->GetIndexType() == IndexType::PRIMARY);
  BOOST_TEST(br2->GetBysValue() == buff);
  BOOST_TEST(br->GetTotalLength() == br2->GetTotalLength());

  BOOST_TEST(br->GetChildPage() == bp);
  BOOST_TEST(br2->GetChildPage() == bp);

  lr->SubmitStatement(stmt, RecordStatus::ROLLBACKED);
  lr->ReleaseLock(&indexTree, false);
  delete lr;
  delete br;
  delete br2;
  bp->SetReferred(false);

  indexTree.Close();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchRecord_UniqueKey_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME1 =
      ROOT_PATH + "/testBranchRecord_PriKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME1 = "testPriTable";
  const string FILE_NAME2 =
      ROOT_PATH + "/testBranchRecord_UniqueKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME2 = "testUniqueTable";
  DataValueLong dvKey(100);
  DataValueLong dvVal(200);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree indexPri;
  indexPri.CreateIndexTree(TABLE_NAME1.c_str(), FILE_NAME1.c_str(), vctKey,
                           vctVal, GetFileId(), IndexType::PRIMARY);

  StatementEx stmt(1, 1);
  vctKey.push_back(dvKey.Clone(true));
  vctVal.push_back(dvVal.Clone(true));
  LeafRecord *lr = new LeafRecord(&indexPri, vctKey, vctVal, 1, &stmt, false);

  DataValueFixChar dvFix("1234567890abcdefghijklmn", 26, 100);
  VectorDataValue vctSec = {dvFix.Clone(), dvKey.Clone()};
  IndexTree indexSec;
  indexSec.CreateIndexTree(TABLE_NAME2.c_str(), FILE_NAME2.c_str(), vctSec,
                           vctKey, GetFileId(), IndexType::UNIQUE);
  vctSec = {dvFix.Clone(true), dvKey.Clone(true)};
  Byte *bys = lr->GetBysValue() + UI16_2_LEN;
  uint16_t lKey = lr->GetKeyLength();
  LeafRecord *lrSec = new LeafRecord(&indexSec, vctSec, bys, lKey,
                                     ActionType::INSERT, 1, &stmt);

  BranchRecord *br = new BranchRecord(IndexType::UNIQUE, lrSec, 20, nullptr);
  Byte buff[1000];
  br->SaveData(buff);

  BranchRecord *br2 = new BranchRecord(IndexType::UNIQUE, buff, nullptr);

  RawKey rkey(vctSec);
  BOOST_TEST(br2->CompareKey(rkey) == 0);

  BOOST_TEST(br2->GetChildPageId() == 20);
  BOOST_TEST(br2->GetValueLength() == 0);
  BOOST_TEST(br->IsSole());
  BOOST_TEST(!br2->IsSole());
  BOOST_TEST(br->EqualPageId(*br2));
  BOOST_TEST(br2->CompareKey(*br) == 0);
  BOOST_TEST(br2->CompareTo(*br) == 0);
  BOOST_TEST(br2->GetBysValue() == buff);
  BOOST_TEST(br->GetTotalLength() == br2->GetTotalLength());

  lr->SubmitStatement(stmt, RecordStatus::COMMITED);
  lr->ReleaseLock(&indexPri, false);
  delete lr;

  lrSec->SubmitStatement(stmt, RecordStatus::COMMITED);
  lrSec->ReleaseLock(&indexSec, false);
  delete lrSec;

  delete br;
  delete br2;

  indexSec.Close();
  indexPri.Close();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(BranchRecord_NonUniqueKey_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME1 =
      ROOT_PATH + "/testBranchRecord_PriKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME1 = "testPriTable";
  const string FILE_NAME2 =
      ROOT_PATH + "/testBranchRecord_NonUniqueKey_test" + StrMSTime() + ".dat";
  const string TABLE_NAME2 = "testNonUniqueKeyTable";

  DataValueLong dvKey(100);
  DataValueLong dvVal(200);
  VectorDataValue vctKey = {dvKey.Clone()};
  VectorDataValue vctVal = {dvVal.Clone()};
  IndexTree indexPri;
  indexPri.CreateIndexTree(TABLE_NAME1.c_str(), FILE_NAME1.c_str(), vctKey,
                           vctVal, GetFileId(), IndexType::PRIMARY);

  StatementEx stmt(1, 1);
  vctKey.push_back(dvKey.Clone(true));
  vctVal.push_back(dvVal.Clone(true));
  LeafRecord *lr = new LeafRecord(&indexPri, vctKey, vctVal, 1, &stmt, false);

  DataValueFixChar dvFix("1234567890abcdefghijklmn", 26, 100);
  VectorDataValue vctSec = {dvFix.Clone(), dvKey.Clone()};
  IndexTree indexSec;
  indexSec.CreateIndexTree(TABLE_NAME2.c_str(), FILE_NAME2.c_str(), vctSec,
                           vctKey, GetFileId(), IndexType::NON_UNIQUE);
  vctSec = {dvFix.Clone(true), dvKey.Clone(true)};
  Byte *bys = lr->GetBysValue() + UI16_2_LEN;
  uint16_t lKey = lr->GetKeyLength();
  LeafRecord *lrSec = new LeafRecord(&indexSec, vctSec, bys, lKey,
                                     ActionType::INSERT, 1, &stmt);

  BranchRecord *br =
      new BranchRecord(IndexType::NON_UNIQUE, lrSec, 20, nullptr);
  Byte buff[1000];
  br->SaveData(buff);

  BranchRecord *br2 = new BranchRecord(IndexType::NON_UNIQUE, buff, nullptr);

  RawKey rkey(vctSec);
  BOOST_TEST(br2->CompareKey(rkey) == 0);

  BOOST_TEST(br2->GetChildPageId() == 20);
  BOOST_TEST(br2->GetValueLength() == 8);
  BOOST_TEST(br->IsSole());
  BOOST_TEST(!br2->IsSole());
  BOOST_TEST(br->EqualPageId(*br2));
  BOOST_TEST(br2->CompareKey(*br) == 0);
  BOOST_TEST(br2->CompareTo(*br) == 0);
  BOOST_TEST(br2->GetBysValue() == buff);
  BOOST_TEST(br->GetTotalLength() == br2->GetTotalLength());

  lr->SubmitStatement(stmt, RecordStatus::COMMITED);
  lr->ReleaseLock(&indexPri, false);
  delete lr;

  lrSec->SubmitStatement(stmt, RecordStatus::COMMITED);
  lrSec->ReleaseLock(&indexSec, false);
  delete lrSec;

  delete br;
  delete br2;

  indexSec.Close();
  indexPri.Close();
  CachePagePool::ClearPool();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
