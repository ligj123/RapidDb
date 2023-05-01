#include "../../src/core/LeafRecord.h"
#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueBlob.h"
#include "../../src/dataType/DataValueDigit.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/dataType/DataValueFixChar.h"
#include "../../src/dataType/DataValueVarChar.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)

BOOST_AUTO_TEST_CASE(LeafRecord_test) {
  const string FILE_NAME = ROOT_PATH + "/testLeafRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100LL);
  DataValueLong *dvVal = new DataValueLong(200LL);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree = new IndexTree();
  bool b = indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1,
                                  IndexType::PRIMARY);
  BOOST_TEST(b);
  vctKey.push_back(dvKey->Clone(true));
  vctVal.push_back(dvVal->Clone(true));
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  BOOST_TEST(8 == lr->GetKeyLength());
  BOOST_TEST(9 == lr->GetValueLength());
  BOOST_TEST(34 == lr->GetTotalLength());
  BOOST_TEST(lr->IsSole());
  BOOST_TEST(!lr->IsTransaction());
  BOOST_TEST(!lr->IsGapLock());

  Byte byArr[512];
  lr->SaveData(byArr);
  LeafRecord *lr2 = new LeafRecord(indexTree, byArr);

  RawKey *key = lr2->GetKey();
  RawKey key2(vctKey);
  BOOST_TEST(*key == key2);
  delete key;

  VectorDataValue vctVal2;
  int hr = lr2->GetListValue(vctVal2);
  BOOST_TEST(hr == 0);
  BOOST_TEST(vctVal2[0]->GetLong() == 200LL);

  BOOST_TEST(lr->CompareTo(*lr2) == 0);
  BOOST_TEST(lr->CompareKey(key2) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  lr->DecRef();
  lr2->DecRef();

  indexTree->Close();

  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(LeafRecordBig_test) {
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
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1,
                         IndexType::PRIMARY);

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  BOOST_TEST(25 == lr->GetKeyLength());
  BOOST_TEST(173 == lr->GetValueLength());
  BOOST_TEST(215 == lr->GetTotalLength());
  BOOST_TEST(lr->IsSole());
  BOOST_TEST(!lr->IsTransaction());
  BOOST_TEST(!lr->IsGapLock());

  Byte byArr[512];
  lr->SaveData(byArr);
  LeafRecord *lr2 = new LeafRecord(indexTree, byArr);

  RawKey rkey(vctKey);
  BOOST_TEST(lr2->CompareKey(rkey) == 0);

  RawKey *key = lr2->GetKey();
  BOOST_TEST(*key == rkey);
  delete key;

  VectorDataValue vctVal2;
  int hr = lr2->GetListValue(vctVal2);
  BOOST_TEST(hr == 0);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob);

  BOOST_TEST(lr->CompareTo(*lr2) == 0);
  BOOST_TEST(lr->CompareKey(rkey) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  lr->DecRef();
  lr2->DecRef();

  string str(18000, 'a');
  DataValueBlob dvBlob2(str.c_str(), (uint32_t)str.size(), 20000);
  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob2.Clone(true)};
  lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);

  lr->SaveData(byArr);
  lr2 = new LeafRecord(indexTree, byArr);
  lr2->FillOverPage();
  BOOST_TEST(lr2->CompareKey(rkey) == 0);

  hr = lr2->GetListValue(vctVal2);
  BOOST_TEST(hr == 0);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob2);

  BOOST_TEST(lr->CompareTo(*lr2) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  lr->DecRef();
  lr2->DecRef();
  indexTree->Close();
}

BOOST_AUTO_TEST_CASE(LeafRecord_Multi_Version_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testLeafRecordMulti_Version" + StrMSTime() + ".dat";
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
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 1,
                         IndexType::PRIMARY);

  HeadPage *hp = indexTree->GetHeadPage();

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  string sblob = p3;
  dvLong = 300;
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  lr->UpdateRecord(vctVal, 2, nullptr, ActionType::UPDATE, false);

  BOOST_TEST(lr->GetVersionNumber() == 1);
  MVector<uint64_t> vct;
  lr->GetVerStamps(vct);
  BOOST_TEST(vct.size() == 1);
  BOOST_TEST(vct[0] == 2);

  VectorDataValue vctDv;
  lr->GetListValue({0, 2}, vctDv, 2);

  BOOST_TEST(vctDv.size() == 2);
  BOOST_TEST(dvLong == *vctDv[0]);
  BOOST_TEST(dvBlob == *vctDv[1]);

  hp->AddNewRecordVersion(10, MicroSecTime());
  vctVal.RemoveAll();
  lr->UpdateRecord(vctVal, 10, nullptr, ActionType::DELETE, false);

  BOOST_TEST(lr->GetVersionNumber() == 2);
  lr->GetVerStamps(vct);
  BOOST_TEST(vct.size() == 2);
  BOOST_TEST(vct[1] == 2);
  BOOST_TEST(vct[0] == 10);

  lr->GetListValue({0, 2}, vctDv, 2);
  BOOST_TEST(vctDv.size() == 2);
  BOOST_TEST(dvLong == *vctDv[0]);
  BOOST_TEST(dvBlob == *vctDv[1]);

  int hr = lr->GetListValue(vctDv);
  BOOST_TEST(hr == 1);
  BOOST_TEST(vctDv.size() == 0);

  hp->AddNewRecordVersion(20, MicroSecTime() + 10);
  dvLong = 400;
  sblob += p3;
  dvBlob.SetValue(sblob.data(), (uint32_t)sblob.size());
  vctVal = {dvLong.Clone(true), dvFix.Clone(false), dvBlob.Clone(true)};
  lr->UpdateRecord(vctVal, 21, nullptr, ActionType::UPDATE, false);

  BOOST_TEST(lr->GetVersionNumber() == 3);
  lr->GetVerStamps(vct);
  BOOST_TEST(vct.size() == 3);
  BOOST_TEST(vct[0] == 21);

  hr = lr->GetListValue(vctDv, 22);
  BOOST_TEST(hr == 0);
  BOOST_TEST(vctDv.size() == 3);
  BOOST_TEST(dvLong == *vctDv[0]);
  BOOST_TEST(vctDv[1]->IsNull());
  BOOST_TEST(dvBlob == *vctDv[2]);

  hr = lr->GetListValue(vctDv, 12);
  BOOST_TEST(hr == 1);

  hp->AddNewRecordVersion(30, MicroSecTime() + 20);
  hp->AddNewRecordVersion(40, MicroSecTime() + 30);

  dvLong = 500;
  sblob = string(18000, 'a');
  dvBlob.SetValue(sblob.data(), (uint32_t)sblob.size());
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  lr->UpdateRecord(vctVal, 40, nullptr, ActionType::UPDATE, false);

  BOOST_TEST(lr->GetVersionNumber() == 4);
  lr->GetVerStamps(vct);
  BOOST_TEST(vct[0] == 40);

  hr = lr->GetListValue(vctDv);
  BOOST_TEST(hr == 0);
  BOOST_TEST(vctDv.size() == 3);
  BOOST_TEST(dvLong == *vctDv[0]);
  BOOST_TEST(dvFix == *vctDv[1]);
  BOOST_TEST(dvBlob == *vctDv[2]);

  hp->AddNewRecordVersion(50, MicroSecTime() + 40);
  hp->AddNewRecordVersion(60, MicroSecTime() + 50);
  hp->AddNewRecordVersion(70, MicroSecTime() + 60);

  dvLong = 600;
  sblob = p3;
  dvBlob.SetValue(sblob.data(), (uint32_t)sblob.size());
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  lr->UpdateRecord(vctVal, 72, nullptr, ActionType::UPDATE, false);

  BOOST_TEST(lr->GetVersionNumber() == 5);
  lr->GetVerStamps(vct);
  BOOST_TEST(vct.size() == 5);
  BOOST_TEST(vct[0] == 72);
  BOOST_TEST(vct[1] == 40);

  hr = lr->GetListValue(vctDv);
  BOOST_TEST(vctDv.size() == 3);
  BOOST_TEST(dvLong == *vctDv[0]);
  BOOST_TEST(dvFix == *vctDv[1]);
  BOOST_TEST(dvBlob == *vctDv[2]);

  hr = lr->GetListValue(vctDv, 60);
  BOOST_TEST(vctDv.size() == 3);
  BOOST_TEST(vctDv[0]->GetLong() == 500);
  BOOST_TEST(dvFix == *vctDv[1]);
  BOOST_TEST(vctDv[2]->GetPersistenceLength(SavePosition::VALUE) == 18000);

  hr = lr->GetListValue(vctDv, 15);
  BOOST_TEST(hr == 1);
  BOOST_TEST(vctDv.size() == 0);

  lr->DecRef(true);
  indexTree->Close();
}

BOOST_AUTO_TEST_CASE(LeafRecord_Second_test) {
  const string FILE_NAME =
      ROOT_PATH + "/testLeafRecordSecond" + StrMSTime() + ".dat";
  const string FILE_NAME2 =
      ROOT_PATH + "/testLeafRecordSecond" + StrMSTime() + "2.dat";
  const string TABLE_NAME = "testTable";
  const string TABLE_NAME2 = "testTable2";

  DataValueInt dvInt(100);
  const char *p1 = "abcdefghijklmnopqrst";
  DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100);
  DataValueLong dvLong(200);

  VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
  VectorDataValue vctVal = {dvLong.Clone()};
  IndexTree *indexTree = new IndexTree();
  indexTree->CreateIndex(TABLE_NAME, FILE_NAME, vctKey, vctVal, 0,
                         IndexType::PRIMARY);

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true)};
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);

  DataValueLong dvLKey(200);
  VectorDataValue vctSec = {dvLKey.Clone(true), dvVar.Clone(true)};
  vctKey = {dvInt.Clone(), dvVar.Clone()};
  IndexTree *secTree = new IndexTree();
  secTree->CreateIndex(TABLE_NAME2, FILE_NAME2, vctSec, vctKey, 1,
                       IndexType::UNIQUE);

  vctSec = {dvLong.Clone(true), dvVar.Clone(true)};
  Byte *bys = lr->GetBysValue() + UI16_2_LEN;
  uint16_t lKey = lr->GetKeyLength();
  LeafRecord *lrSec =
      new LeafRecord(secTree, vctSec, bys, lKey, ActionType::INSERT, nullptr);

  RawKey rkey(vctSec);
  BOOST_TEST(lrSec->CompareKey(rkey) == 0);

  RawKey *key = lrSec->GetPrimayKey();
  BOOST_TEST(lr->CompareKey(*key) == 0);
  delete key;

  lrSec->DecRef();
  lr->DecRef();
  indexTree->Close();
  secTree->Close();
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
