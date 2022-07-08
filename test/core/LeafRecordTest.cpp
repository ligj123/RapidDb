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
#include "CoreSuit.h"
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace storage {
namespace fs = std::filesystem;
BOOST_FIXTURE_TEST_SUITE(CoreTest, SuiteFixture)

BOOST_AUTO_TEST_CASE(LeafRecord_test) {
  const string FILE_NAME = "./dbTest/testLeafRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong *dvKey = new DataValueLong(100LL, true);
  DataValueLong *dvVal = new DataValueLong(200LL, false);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  IndexTree *indexTree =
      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal, IndexType::PRIMARY);

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

  VectorDataValue vctKey2;
  lr2->GetListKey(vctKey2);
  BOOST_TEST(vctKey2[0]->GetLong() == 100LL);
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

  lr->ReleaseRecord();
  lr2->ReleaseRecord();

  indexTree->Close();

  delete dvKey;
  delete dvVal;
}

BOOST_AUTO_TEST_CASE(LeafRecordBig_test) {
  const string FILE_NAME = "./dbTest/testLeafRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueInt dvInt(100, true);
  const char *p1 = "abcdefghijklmnopqrst";
  DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100, true);
  DataValueLong dvLong(200, false);
  const char *p2 = "abcdefghijklmnopqrst1234567890";
  DataValueFixChar dvFix(p2, (uint32_t)strlen(p2), 100, false);
  const char *p3 =
      "abcdefghijklmnopqrst1234567890abcdefghijklmnopqrst1234567890";
  DataValueBlob dvBlob(p3, (uint32_t)strlen(p3), 20000, false);

  VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
  VectorDataValue vctVal = {dvLong.Clone(), dvFix.Clone(), dvBlob.Clone()};
  IndexTree *indexTree =
      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal, IndexType::PRIMARY);

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  BOOST_TEST(27 == lr->GetKeyLength());
  BOOST_TEST(173 == lr->GetValueLength());
  BOOST_TEST(217 == lr->GetTotalLength());
  BOOST_TEST(lr->IsSole());
  BOOST_TEST(!lr->IsTransaction());
  BOOST_TEST(!lr->IsGapLock());

  Byte byArr[512];
  lr->SaveData(byArr);
  LeafRecord *lr2 = new LeafRecord(indexTree, byArr);

  VectorDataValue vctKey2;
  lr2->GetListKey(vctKey2);
  BOOST_TEST(*vctKey2[0] == dvInt);
  BOOST_TEST(*vctKey2[1] == dvVar);

  RawKey *key = lr2->GetKey();
  RawKey key2(vctKey);
  BOOST_TEST(*key == key2);
  delete key;

  VectorDataValue vctVal2;
  int hr = lr2->GetListValue(vctVal2);
  BOOST_TEST(hr == 0);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob);

  BOOST_TEST(lr->CompareTo(*lr2) == 0);
  BOOST_TEST(lr->CompareKey(key2) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  lr->ReleaseRecord();
  lr2->ReleaseRecord();

  string str(18000, 'a');
  DataValueBlob dvBlob2(str.c_str(), (uint32_t)str.size(), 20000, false);
  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob2.Clone(true)};
  lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);

  lr->SaveData(byArr);
  lr2 = new LeafRecord(indexTree, byArr);
  lr2->FillOverPage();
  lr2->GetListKey(vctKey2);
  BOOST_TEST(*vctKey2[0] == dvInt);
  BOOST_TEST(*vctKey2[1] == dvVar);

  hr = lr2->GetListValue(vctVal2);
  BOOST_TEST(hr == 0);
  BOOST_TEST(*vctVal2[0] == dvLong);
  BOOST_TEST(*vctVal2[1] == dvFix);
  BOOST_TEST(*vctVal2[2] == dvBlob2);

  BOOST_TEST(lr->CompareTo(*lr2) == 0);
  BOOST_TEST(lr->CompareKey(*lr2) == 0);

  lr->ReleaseRecord();
  lr2->ReleaseRecord();
  indexTree->Close();
}

BOOST_AUTO_TEST_CASE(LeafRecord_Multi_Version_test) {
  const string FILE_NAME = "./dbTest/testLeafRecord" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueInt dvInt(100, true);
  const char *p1 = "abcdefghijklmnopqrst";
  DataValueVarChar dvVar(p1, (uint32_t)strlen(p1), 100, true);
  DataValueLong dvLong(200, false);
  const char *p2 = "abcdefghijklmnopqrst1234567890";
  DataValueFixChar dvFix(p2, (uint32_t)strlen(p2), 100, false);
  const char *p3 =
      "abcdefghijklmnopqrst1234567890abcdefghijklmnopqrst1234567890";
  DataValueBlob dvBlob(p3, (uint32_t)strlen(p3), 20000, false);

  VectorDataValue vctKey = {dvInt.Clone(), dvVar.Clone()};
  VectorDataValue vctVal = {dvLong.Clone(), dvFix.Clone(), dvBlob.Clone()};
  IndexTree *indexTree =
      new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal, IndexType::PRIMARY);

  HeadPage *hp = indexTree->GetHeadPage();

  vctKey = {dvInt.Clone(true), dvVar.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  LeafRecord *lr = new LeafRecord(indexTree, vctKey, vctVal, 1, nullptr);
  string sblob = p3;
  dvLong = 300;
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
  vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};

  for (int i = 0; i < 7; i++) {
    sblob += p3;
    vctVal = {dvLong.Clone(true), dvFix.Clone(true), dvBlob.Clone(true)};
    lr->UpdateRecord()
  }
}

// BOOST_AUTO_TEST_CASE(LeafRecord_Block_test) {
//  const string FILE_NAME = "./dbTest/testLeafRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  char *blockData = new char[1024 * 10];
//  for (int i = 0; i < 1024 * 10; i++) {
//    blockData[i] = (Byte)i;
//  }
//
//  char *blockData2 = new char[1024 * 20];
//  for (int i = 0; i < 1024 * 20; i++) {
//    blockData2[i] = (Byte)i;
//  }
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal1 = new DataValueLong(200, false);
//  DataValueBlob *dvVal2 = new DataValueBlob(1024 * 20, false);
//  VectorDataValue vctKey = {dvKey->Clone(false)};
//  VectorDataValue vctVal = {dvVal1->Clone(false), dvVal2->Clone(false)};
//
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//
//  vctKey.push_back(dvKey->Clone(true));
//  vctVal.push_back(dvVal1->Clone(true));
//  vctVal.push_back(new DataValueBlob(blockData, 1024 * 10, 1024 * 20, false));
//
//  LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, 1);
//
//  PageFile *ovf = indexTree->GetOverflowFile();
//  uint64_t lenOvf = ovf->Length();
//
//  LeafRecord *rr2 = new LeafRecord(indexTree, vctKey, vctVal, 2);
//  uint64_t lenOvf2 = ovf->Length();
//  BOOST_TEST(lenOvf <= lenOvf2);
//
//  LeafRecord *rr3 =
//      new LeafRecord(indexTree, vctKey, vctVal, 3, ActionType::UNKNOWN, 0,
//      rr2);
//  uint64_t lenOvf3 = ovf->Length();
//  BOOST_TEST(lenOvf2 == lenOvf3);
//
//  delete vctVal[1];
//  vctVal[1] = new DataValueBlob(blockData2, 1024 * 20, 1024 * 20, false);
//  LeafRecord *rr4 =
//      new LeafRecord(indexTree, vctKey, vctVal, 4, ActionType::UNKNOWN, 0,
//      rr);
//  uint64_t lenOvf4 = ovf->Length();
//  BOOST_TEST(lenOvf3 < lenOvf4);
//
//  delete dvKey;
//  delete dvVal1;
//  delete dvVal2;
//  rr->ReleaseRecord();
//  rr2->ReleaseRecord();
//  rr3->ReleaseRecord();
//  rr4->ReleaseRecord();
//  indexTree->Close(true);
//
//  fs::remove(fs::path(FILE_NAME));
//  string name =
//      FILE_NAME.substr(0, FILE_NAME.find_last_of('/')) + "/overfile.dat";
//  fs::remove(fs::path(name));
//}
//
// BOOST_AUTO_TEST_CASE(LeafRecord_Snapshot_test) {
//  const string FILE_NAME = "./dbTest/testLeafRecord" + StrMSTime() + ".dat";
//  const string TABLE_NAME = "testTable";
//  char *blockData1 = new char[1024 * 2];
//  for (int i = 0; i < 1024 * 2; i++) {
//    blockData1[i] = (Byte)i;
//  }
//
//  char *blockData2 = new char[1024 * 5];
//  for (int i = 0; i < 1024 * 5; i++) {
//    blockData2[i] = (Byte)i;
//  }
//
//  char *blockData3 = new char[1024 * 10];
//  for (int i = 0; i < 1024 * 10; i++) {
//    blockData3[i] = (Byte)i;
//  }
//
//  DataValueLong *dvKey = new DataValueLong(100, true);
//  DataValueLong *dvVal1 = new DataValueLong(200, false);
//  DataValueBlob *dvVal2 = new DataValueBlob(1024 * 20, false);
//  VectorDataValue vctKey = {dvKey->Clone(false)};
//  VectorDataValue vctVal = {dvVal1->Clone(false), dvVal2->Clone(false)};
//
//  IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
//  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
//  indexTree->GetHeadPage()->AddNewRecordVersion(100);
//
//  vctKey.push_back(dvKey->Clone(true));
//  vctVal.push_back(dvVal1->Clone(true));
//  vctVal.push_back(new DataValueBlob(blockData1, 1024 * 2, 1024 * 2, false));
//
//  LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, 1);
//  LeafRecord *rr2 = new LeafRecord(indexTree, vctKey, vctVal, 200,
//                                   ActionType::UNKNOWN, 0, rr);
//
//  PriValStruct *valStru = rr2->GetPriValStruct();
//  BOOST_TEST(valStru->bLastOvf == false);
//  BOOST_TEST(valStru->bOvf == false);
//  BOOST_TEST(valStru->verCount == 2);
//  BOOST_TEST(valStru->pageValOffset == 33);
//  BOOST_TEST(valStru->arrStamp[0] == 200);
//  BOOST_TEST(valStru->arrStamp[1] == 1);
//  BOOST_TEST(valStru->arrPageLen[0] == 2062);
//  BOOST_TEST(valStru->arrPageLen[1] == 4124);
//
//  indexTree->GetHeadPage()->AddNewRecordVersion(250);
//  delete vctVal[1];
//  vctVal[1] = new DataValueBlob(blockData2, 1024 * 5, 1024 * 5, false);
//  LeafRecord *rr3 = new LeafRecord(indexTree, vctKey, vctVal, 300,
//                                   ActionType::UNKNOWN, 0, rr2);
//
//  valStru = rr3->GetPriValStruct();
//  BOOST_TEST(valStru->bLastOvf == false);
//  BOOST_TEST(valStru->bOvf == true);
//  BOOST_TEST(valStru->verCount == 3);
//  BOOST_TEST(valStru->pageValOffset == 65);
//  BOOST_TEST(valStru->arrStamp[0] == 300);
//  BOOST_TEST(valStru->arrStamp[1] == 200);
//  BOOST_TEST(valStru->arrStamp[2] == 1);
//  BOOST_TEST(valStru->ovfOffset == 0);
//  BOOST_TEST(valStru->ovfRange == 8192);
//  BOOST_TEST(valStru->indexOvfStart == 2);
//  BOOST_TEST(valStru->lenInPage == 5134);
//  BOOST_TEST(valStru->arrOvfLen[0] == 0);
//  BOOST_TEST(valStru->arrOvfLen[1] == 2062);
//  BOOST_TEST(valStru->arrOvfLen[2] == 4124);
//
//  delete vctVal[1];
//  vctVal[1] = new DataValueBlob(blockData3, 1024 * 10, 1024 * 10, false);
//  LeafRecord *rr4 = new LeafRecord(indexTree, vctKey, vctVal, 400,
//                                   ActionType::UNKNOWN, 0, rr3);
//
//  valStru = new PriValStruct(rr4->GetBysValue());
//  BOOST_TEST(valStru->bLastOvf == true);
//  BOOST_TEST(valStru->bOvf == true);
//  BOOST_TEST(valStru->verCount == 3);
//  BOOST_TEST(valStru->pageValOffset == 65);
//  BOOST_TEST(valStru->arrStamp[0] == 400);
//  BOOST_TEST(valStru->arrStamp[1] == 200);
//  BOOST_TEST(valStru->arrStamp[2] == 1);
//  BOOST_TEST(valStru->ovfOffset == 8192);
//  BOOST_TEST(valStru->ovfRange == 16384);
//  BOOST_TEST(valStru->indexOvfStart == 1);
//  BOOST_TEST(valStru->lenInPage == 9);
//  BOOST_TEST(valStru->arrOvfLen[0] == 10245);
//  BOOST_TEST(valStru->arrOvfLen[1] == 12307);
//  BOOST_TEST(valStru->arrOvfLen[2] == 14369);
//
//  VectorDataValue vct;
//  rr4->GetListKey(vct);
//  BOOST_TEST(*dvKey == *vct[0]);
//
//  vct.RemoveAll();
//  rr4->GetListValue(vct, 100);
//  BOOST_TEST(*dvVal1 == *vct[0]);
//  DataValueBlob *dvBlob = (DataValueBlob *)vct[1];
//  BOOST_TEST(2048 == dvBlob->GetDataLength());
//  const char *p = (const char *)(*dvBlob);
//  for (int i = 0; i < 2048; i++) {
//    assert((char)i == p[i]);
//  }
//
//  vct.RemoveAll();
//  rr4->GetListValue(vct);
//  rr4->GetListOverflow(vct);
//  BOOST_TEST(*dvVal1 == *vct[0]);
//  dvBlob = (DataValueBlob *)vct[1];
//  BOOST_TEST(10240 == dvBlob->GetDataLength());
//  p = (const char *)(*dvBlob);
//  for (int i = 0; i < 10240; i++) {
//    assert((char)i == p[i]);
//  }
//
//  delete dvKey;
//  delete dvVal1;
//  delete dvVal2;
//  rr->ReleaseRecord();
//  rr2->ReleaseRecord();
//  rr3->ReleaseRecord();
//  rr4->ReleaseRecord();
//  indexTree->Close(true);
//
//  fs::remove(fs::path(FILE_NAME));
//  string name =
//      FILE_NAME.substr(0, FILE_NAME.find_last_of('/')) + "/overfile.dat";
//  fs::remove(fs::path(name));
//}

BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
