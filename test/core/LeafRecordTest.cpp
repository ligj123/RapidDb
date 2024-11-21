#include "../../src/core/LeafRecord.h"
#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueBlob.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/dataType/DataValueFixChar.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/pool/FilePagePool.h"
#include "../../src/statement/Statement.h"
#include "../../src/utils/BytesFuncs.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;

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

BOOST_AUTO_TEST_CASE(LeafRecord_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME = ROOT_PATH + "/testLeafRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100LL);
  DataValueLong *dvVal = new DataValueLong(200LL);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree indexTree;
  bool b =
      indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                                vctVal, GetFileId(), IndexType::PRIMARY);
  BOOST_TEST(b);
  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));

  StatementEx stmt(1, 1);
  LeafRecord *lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, &stmt, false);
  BOOST_TEST(8 == lr->GetKeyLength());
  BOOST_TEST(9 == lr->GetValueLength());
  BOOST_TEST(34 == lr->GetTotalLength());
  BOOST_TEST(lr->IsSole());
  BOOST_TEST(!lr->IsGapLock());
  BOOST_TEST(!lr->ReleaseLockAble());
  RawKey key(vctKey);
  BOOST_TEST(lr->GetKey().CompareTo(key) == 0);
  BOOST_TEST(!lr->IsStable());
  BOOST_TEST(!lr->HasOverflowPage());
  BOOST_TEST(lr->GetAction() == ActionType::INSERT);

  lr->SubmitStatement(stmt, RecordStatus::COMMITED);
  BOOST_TEST(lr->IsStable());
  BOOST_TEST(lr->ReleaseLock(&indexTree, false) == ReleaseResult::FISHED);
  BOOST_TEST(lr->IsStable());
  BOOST_TEST(lr->GetAction() == ActionType::NO_ACTION);

  Byte byArr[512];
  lr->SaveData(byArr);
  LeafRecord *lr2 = new LeafRecord(IndexType::PRIMARY, byArr);

  BOOST_TEST(key == lr2->GetKey());
  BOOST_TEST(8 == lr2->GetKeyLength());
  BOOST_TEST(9 == lr2->GetValueLength());
  BOOST_TEST(34 == lr2->GetTotalLength());
  BOOST_TEST(!lr2->IsSole());

  VectorDataValue vctVal2;
  ReadResult hr = lr2->ReadListValue({}, vctVal2, &indexTree);
  BOOST_TEST(hr == ReadResult::OK_NOLOCK);
  BOOST_TEST(vctVal2[0]->GetLong() == 200LL);

  BOOST_TEST(lr->CompareKey(key) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  delete lr;
  delete lr2;

  indexTree.Close();
  CachePagePool::ClearPool();

  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(LeafRecordBig_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testLeafRecordBig" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueInt dvInt(100);
  const char *p1 = "abcdefghijklmnopqrst";
  DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100);
  DataValueLong dvLong(200);
  const char *p2 = "abcdefghijklmnopqrst1234567890";
  DataValueFixChar dvFix(p2, (uint32_t)strlen(p2), 100);
  const char *p3 =
      "abcdefghijklmnopqrst1234567890abcdefghijklmnopqrst1234567890";
  DataValueBlob dvBlob(p3, (uint32_t)strlen(p3), 20000);

  VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
  VectorDataValue vctVal = {dvLong.Clone(), dvFix.Clone(), dvBlob.Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};

  StatementEx stmt(1, 1);
  LeafRecord *lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, &stmt, false);

  BOOST_TEST(25 == lr->GetKeyLength());
  BOOST_TEST(173 == lr->GetValueLength());
  BOOST_TEST(215 == lr->GetTotalLength());
  BOOST_TEST(lr->IsSole());
  BOOST_TEST(!lr->IsGapLock());

  lr->SubmitStatement(stmt, RecordStatus::COMMITED);
  BOOST_TEST(lr->ReleaseLock(&indexTree, false) == ReleaseResult::FISHED);

  Byte byArr[512];
  lr->SaveData(byArr);
  LeafRecord *lr2 = new LeafRecord(IndexType::PRIMARY, byArr);

  RawKey rkey(vctKey);
  BOOST_TEST(lr2->CompareKey(rkey) == 0);

  VectorDataValue vctVal2;
  ReadResult hr = lr2->ReadListValue({}, vctVal2, &indexTree);
  BOOST_TEST(hr == ReadResult::OK_NOLOCK);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob);

  BOOST_TEST(lr->CompareKey(rkey) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  delete lr;
  delete lr2;

  string str(18000, 'a');
  DataValueBlob dvBlob2(str.c_str(), (uint32_t)str.size(), 20000);
  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob2.Clone(true)};
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);

  vctVal2.clear();
  hr = lr->ReadListValue({}, vctVal2, &indexTree);
  BOOST_TEST(hr == ReadResult::OK_NOLOCK);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob2);

  lr->SaveData(byArr);
  FilePagePool::SyncWritePage(lr->GetOverflowPage());

  lr2 = new LeafRecord(IndexType::PRIMARY, byArr);
  lr2->LoadOverflowPage(&indexTree, true);
  BOOST_TEST(lr2->CompareKey(rkey) == 0);

  vctVal2.clear();
  hr = lr2->ReadListValue({}, vctVal2, &indexTree);
  BOOST_TEST(hr == ReadResult::OK_NOLOCK);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob2);

  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  delete lr;
  delete lr2;
  indexTree.Close();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(LeafRecord_SecIndex_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testLeafRecordSecIndex" + StrMSTime() + ".dat";
  const string FILE_NAME2 =
      ROOT_PATH + "/testLeafRecordSecIndex" + StrMSTime() + "2.dat";
  const string TABLE_NAME = "testTable";
  const string TABLE_NAME2 = "testTable2";

  DataValueInt dvInt(100);
  const char *p1 = "abcdefghijklmnopqrst";
  DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100);
  DataValueLong dvLong(200);

  VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
  VectorDataValue vctVal = {dvLong.Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true)};
  LeafRecord *lr =
      new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);

  DataValueLong dvLKey(200);
  VectorDataValue vctSec = {dvLKey.Clone(true), dvVar.Clone(true)};
  vctKey = {dvInt.Clone(), dvVar.Clone()};
  IndexTree secTree;
  secTree.CreateIndexTree(TABLE_NAME2.c_str(), FILE_NAME2.c_str(), vctSec,
                          vctKey, GetFileId(), IndexType::UNIQUE);

  vctSec = {dvLong.Clone(true), dvVar.Clone(true)};
  Byte *bys = lr->GetBysValue() + UI16_2_LEN;
  uint16_t lKey = lr->GetKeyLength();
  LeafRecord *lrSec =
      new LeafRecord(&secTree, vctSec, bys, lKey, ActionType::INSERT, 1);

  RawKey rkey(vctSec);
  BOOST_TEST(lrSec->CompareKey(rkey) == 0);

  RawKey key = lrSec->GetPrimayKey();
  BOOST_TEST(lr->CompareKey(key) == 0);

  delete lrSec;
  delete lr;
  indexTree.Close();
  secTree.Close();
  CachePagePool::ClearPool();
}

BOOST_AUTO_TEST_CASE(LeafRecord_Update_Read_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const string FILE_NAME =
      ROOT_PATH + "/testLeafRecordUpdate" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueInt dvInt(100);
  const char *p1 = "abcdefghijklmnopqrst";
  DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100);
  DataValueLong dvLong(200);
  const char *p2 = "abcdefghijklmnopqrst1234567890";
  DataValueFixChar dvFix(p2, (uint32_t)strlen(p2), 100);
  const char *p3 =
      "abcdefghijklmnopqrst1234567890abcdefghijklmnopqrst1234567890";
  DataValueBlob dvBlob(p3, (uint32_t)strlen(p3), 1000);

  VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
  VectorDataValue vctVal = {dvLong.Clone(), dvFix.Clone(), dvBlob.Clone()};
  IndexTree indexTree;
  indexTree.CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
                            vctVal, GetFileId(), IndexType::PRIMARY);

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};

  // A transaction repeat to update a record.
  StatementEx stmt(1, 1);
  LeafRecord *lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, &stmt, false);

  ((DataValueLong *)vctVal[0])->SetValue(300);
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  StatementEx stmt2(2, 1);
  LeafRecord *lr2 = lr->UpdateRecord(&indexTree, vctVal, 2, &stmt2,
                                     ActionType::UPDATE, false, false);

  VectorDataValue vctDv;
  ReadResult res = lr2->ReadListValue({}, vctDv, &indexTree, nullptr);
  BOOST_TEST(res == ReadResult::LOCKED);

  StatementEx stmt3(3, 1);
  vctDv.clear();
  res =
      lr2->ReadListValue({}, vctDv, &indexTree, &stmt3, ActionType::READ_SHARE);
  BOOST_TEST(res == ReadResult::OK_NOLOCK);

  BOOST_TEST(*vctDv[0] == *vctVal[0]);
  BOOST_TEST(*vctDv[1] == *vctVal[1]);
  BOOST_TEST(*vctDv[2] == *vctVal[2]);

  StatementEx stmt4(1, 2);
  vctDv.clear();
  res =
      lr2->ReadListValue({}, vctDv, &indexTree, &stmt4, ActionType::READ_SHARE);
  BOOST_TEST(res == ReadResult::LOCKED);

  BOOST_TEST(!lr2->ReleaseLockAble());
  lr2->SubmitStatement(stmt2, RecordStatus::COMMITED);
  lr->SubmitStatement(stmt, RecordStatus::COMMITED);
  BOOST_TEST(lr2->ReleaseLockAble());

  ReleaseResult rres = lr2->ReleaseLock(&indexTree, false);
  BOOST_TEST(rres == ReleaseResult::FISHED);

  vctDv.clear();
  res = lr2->ReadListValue({}, vctDv, &indexTree);
  BOOST_TEST(res == ReadResult::OK_NOLOCK);

  BOOST_TEST(vctDv[0]->GetLong() == 200);
  BOOST_TEST(*vctDv[1] == *vctVal[1]);
  BOOST_TEST(*vctDv[2] == *vctVal[2]);

  delete lr2;

  // Delete stable record
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
  lr2 = lr->UpdateRecord(&indexTree, {}, 2, &stmt, ActionType::DELETE, false,
                         false);

  vctDv.clear();
  res =
      lr2->ReadListValue({}, vctDv, &indexTree, &stmt2, ActionType::READ_SHARE);
  BOOST_TEST(res == ReadResult::REC_DELETE);

  lr2->SubmitStatement(stmt, RecordStatus::COMMITED);
  rres = lr2->ReleaseLock(&indexTree, false);
  BOOST_TEST(rres == ReleaseResult::DELETED);
  BOOST_TEST(lr2->GetLock() == nullptr);
  delete lr2;

  // Update stable record
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 1, nullptr, false);
  ((DataValueLong *)vctVal[0])->SetValue(400);
  lr2 = lr->UpdateRecord(&indexTree, vctVal, 2, &stmt, ActionType::DELETE,
                         false, false);
  BOOST_TEST(lr2->GetLock()->_undoRec == lr);

  vctDv.clear();
  res = lr2->ReadListValue({{0, 0}}, vctDv, &indexTree);
  BOOST_TEST(res == ReadResult::OK_NOLOCK);

  BOOST_TEST(vctDv.size() == 1);
  BOOST_TEST(vctDv[0]->GetLong() == 200);

  vctDv.clear();
  res = lr2->ReadListValue({{0, 0}}, vctDv, &indexTree, &stmt4,
                           ActionType::READ_SHARE);
  BOOST_TEST(res == ReadResult::LOCKED);

  vctDv.clear();
  res = lr2->ReadListValue({{0, 0}, {2, 1}}, vctDv, &indexTree);
  BOOST_TEST(res == ReadResult::OK_NOLOCK);
  BOOST_TEST(vctDv.size() == 2);
  BOOST_TEST(vctDv[0]->GetLong() == 200);
  lr2->SubmitStatement(stmt, RecordStatus::COMMITED);
  rres = lr2->ReleaseLock(&indexTree, false);
  delete lr2;

  // Test ReadShare, ReadUpdate
  lr = new LeafRecord(&indexTree, vctKey, vctVal, 10, nullptr, false);
  vctDv.clear();
  res = lr->ReadListValue({}, vctDv, &indexTree, &stmt, ActionType::READ_SHARE);
  BOOST_TEST(res == ReadResult::OK_LOCK);
  BOOST_TEST(vctDv.size() == 3);
  BOOST_TEST(*vctDv[0] == *vctVal[0]);
  BOOST_TEST(*vctDv[1] == *vctVal[1]);
  BOOST_TEST(*vctDv[2] == *vctVal[2]);

  vctDv.clear();
  res =
      lr->ReadListValue({}, vctDv, &indexTree, &stmt4, ActionType::READ_SHARE);
  BOOST_TEST(res == ReadResult::OK_LOCK);
  BOOST_TEST(vctDv.size() == 3);
  BOOST_TEST(*vctDv[0] == *vctVal[0]);
  BOOST_TEST(*vctDv[1] == *vctVal[1]);
  BOOST_TEST(*vctDv[2] == *vctVal[2]);

  vctDv.clear();
  res =
      lr->ReadListValue({}, vctDv, &indexTree, &stmt, ActionType::READ_UPDATE);
  BOOST_TEST(res == ReadResult::LOCKED);

  lr->SubmitStatement(stmt4, RecordStatus::FREEED);
  vctDv.clear();
  res = lr->ReadListValue({{0, 0}, {UINT32_MAX, 1}}, vctDv, &indexTree, &stmt3,
                          ActionType::READ_UPDATE);
  BOOST_TEST(res == ReadResult::OK_NOLOCK);
  BOOST_TEST(vctDv.size() == 2);
  BOOST_TEST(*vctDv[0] == *vctVal[0]);
  BOOST_TEST(vctDv[1]->GetLong() == 10);
  lr->SubmitStatement(stmt3, RecordStatus::FREEED);
  lr->ReleaseLock(&indexTree, false);
  delete lr;

  indexTree.Close();
  CachePagePool::ClearPool();
}

// BOOST_AUTO_TEST_CASE(LeafRecord_Multi_Version_test) {
//   const string FILE_NAME =
//       ROOT_PATH + "/testLeafRecordMulti_Version" + StrMSTime() + ".dat";
//   const string TABLE_NAME = "testTable";

//   DataValueInt dvInt(100);
//   const char *p1 = "abcdefghijklmnopqrst";
//   DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100);
//   DataValueLong dvLong(200);
//   const char *p2 = "abcdefghijklmnopqrst1234567890";
//   DataValueFixChar dvFix(p2, (uint32_t)strlen(p2), 100);
//   const char *p3 =
//       "abcdefghijklmnopqrst1234567890abcdefghijklmnopqrst1234567890";
//   DataValueBlob dvBlob(p3, (uint32_t)strlen(p3), 20000);

//   VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
//   VectorDataValue vctVal = {dvLong.Clone(), dvFix.Clone(), dvBlob.Clone()};
//   IndexTree *indexTree = new IndexTree();
//   indexTree->CreateIndexTree(TABLE_NAME.c_str(), FILE_NAME.c_str(), vctKey,
//                              vctVal, 1, IndexType::PRIMARY);

//   HeadPage *hp = indexTree->GetHeadPage();

//   vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
//   vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
//   LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
//   string sblob = p3;
//   dvLong = 300;
//   vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
//   lr->UpdateRecord(vctVal, 2, nullptr, ActionType::UPDATE, false);

//   BOOST_TEST(lr->GetVersionNumber() == 1);
//   MVector<uint64_t> vct;
//   lr->GetVerStamps(vct);
//   BOOST_TEST(vct.size() == 1);
//   BOOST_TEST(vct[0] == 2);

//   VectorDataValue vctDv;
//   lr->GetListValue({0, 2}, vctDv, 2);

//   BOOST_TEST(vctDv.size() == 2);
//   BOOST_TEST(dvLong == *vctDv[0]);
//   BOOST_TEST(dvBlob == *vctDv[1]);

//   hp->AddNewRecordVersion(10, MicroSecTime());
//   vctVal.clear();
//   lr->UpdateRecord(vctVal, 10, nullptr, ActionType::DELETE, false);

//   BOOST_TEST(lr->GetVersionNumber() == 2);
//   lr->GetVerStamps(vct);
//   BOOST_TEST(vct.size() == 2);
//   BOOST_TEST(vct[1] == 2);
//   BOOST_TEST(vct[0] == 10);

//   lr->GetListValue({0, 2}, vctDv, 2);
//   BOOST_TEST(vctDv.size() == 2);
//   BOOST_TEST(dvLong == *vctDv[0]);
//   BOOST_TEST(dvBlob == *vctDv[1]);

//   int hr = lr->GetListValue(vctDv);
//   BOOST_TEST(hr == 1);
//   BOOST_TEST(vctDv.size() == 0);

//   hp->AddNewRecordVersion(20, MicroSecTime() + 10);
//   dvLong = 400;
//   sblob += p3;
//   dvBlob.SetValue(sblob.data(), (uint32_t)sblob.size());
//   vctVal = {dvLong.Clone(true), dvFix.Clone(false), dvBlob.Clone(true)};
//   lr->UpdateRecord(vctVal, 21, nullptr, ActionType::UPDATE, false);

//   BOOST_TEST(lr->GetVersionNumber() == 3);
//   lr->GetVerStamps(vct);
//   BOOST_TEST(vct.size() == 3);
//   BOOST_TEST(vct[0] == 21);

//   hr = lr->GetListValue(vctDv, 22);
//   BOOST_TEST(hr == 0);
//   BOOST_TEST(vctDv.size() == 3);
//   BOOST_TEST(dvLong == *vctDv[0]);
//   BOOST_TEST(vctDv[1]->IsNull());
//   BOOST_TEST(dvBlob == *vctDv[2]);

//   hr = lr->GetListValue(vctDv, 12);
//   BOOST_TEST(hr == 1);

//   hp->AddNewRecordVersion(30, MicroSecTime() + 20);
//   hp->AddNewRecordVersion(40, MicroSecTime() + 30);

//   dvLong = 500;
//   sblob = string(18000, 'a');
//   dvBlob.SetValue(sblob.data(), (uint32_t)sblob.size());
//   vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
//   lr->UpdateRecord(vctVal, 40, nullptr, ActionType::UPDATE, false);

//   BOOST_TEST(lr->GetVersionNumber() == 4);
//   lr->GetVerStamps(vct);
//   BOOST_TEST(vct[0] == 40);

//   hr = lr->GetListValue(vctDv);
//   BOOST_TEST(hr == 0);
//   BOOST_TEST(vctDv.size() == 3);
//   BOOST_TEST(dvLong == *vctDv[0]);
//   BOOST_TEST(dvFix == *vctDv[1]);
//   BOOST_TEST(dvBlob == *vctDv[2]);

//   hp->AddNewRecordVersion(50, MicroSecTime() + 40);
//   hp->AddNewRecordVersion(60, MicroSecTime() + 50);
//   hp->AddNewRecordVersion(70, MicroSecTime() + 60);

//   dvLong = 600;
//   sblob = p3;
//   dvBlob.SetValue(sblob.data(), (uint32_t)sblob.size());
//   vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
//   lr->UpdateRecord(vctVal, 72, nullptr, ActionType::UPDATE, false);

//   BOOST_TEST(lr->GetVersionNumber() == 5);
//   lr->GetVerStamps(vct);
//   BOOST_TEST(vct.size() == 5);
//   BOOST_TEST(vct[0] == 72);
//   BOOST_TEST(vct[1] == 40);

//   hr = lr->GetListValue(vctDv);
//   BOOST_TEST(vctDv.size() == 3);
//   BOOST_TEST(dvLong == *vctDv[0]);
//   BOOST_TEST(dvFix == *vctDv[1]);
//   BOOST_TEST(dvBlob == *vctDv[2]);

//   hr = lr->GetListValue(vctDv, 60);
//   BOOST_TEST(vctDv.size() == 3);
//   BOOST_TEST(vctDv[0]->GetLong() == 500);
//   BOOST_TEST(dvFix == *vctDv[1]);
//   BOOST_TEST(vctDv[2]->GetPersistenceLength(SavePosition::VALUE) == 18000);

//   hr = lr->GetListValue(vctDv, 15);
//   BOOST_TEST(hr == 1);
//   BOOST_TEST(vctDv.size() == 0);

//   lr->DecRef(true);
//   indexTree->Close();
// }

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
