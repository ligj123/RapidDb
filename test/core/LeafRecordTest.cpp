#include <boost/test/unit_test.hpp>
#include  <filesystem>
#include "../../src/core/LeafRecord.h"
#include "../../src/utils/Utilitys.h"
#include "../../src/dataType/DataValueLong.h"
#include "../../src/core/IndexTree.h"
#include "../../src/utils/BytesConvert.h"
#include "../../src/dataType/DataValueBlob.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/core/LeafPage.h"

namespace storage {
namespace fs = std::filesystem;
BOOST_AUTO_TEST_SUITE(CoreTest)

BOOST_AUTO_TEST_CASE(LeafRecord_test)
{
  const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  DataValueLong* dvKey = new DataValueLong(100LL, true);
  DataValueLong* dvVal = new DataValueLong(200LL, false);
  VectorDataValue vctKey = { dvKey->CloneDataValue() };
  VectorDataValue vctVal = { dvVal->CloneDataValue() };
  IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);

  vctKey.push_back(dvKey->CloneDataValue(true));
  vctVal.push_back(dvVal->CloneDataValue(true));
  LeafRecord* lr = new LeafRecord(indexTree, vctKey, vctVal, 1);
  BOOST_TEST(8 == lr->GetKeyLength());
  BOOST_TEST(20 == lr->GetValueLength());
  BOOST_TEST(32 == lr->GetTotalLength());

  PriValStruct* valStru = lr->GetPriValStruct();
  BOOST_TEST(valStru->bLastOvf == false);
  BOOST_TEST(valStru->bOvf == false);
  BOOST_TEST(valStru->verCount == 1);
  BOOST_TEST(valStru->pageValOffset == 23);
  BOOST_TEST(valStru->arrStamp[0] == 1);
  BOOST_TEST(valStru->arrPageLen[0] == 9);

  VectorDataValue vctKey2;
  lr->GetListKey(vctKey2);
  BOOST_TEST(1 == vctKey2.size());
  BOOST_TEST(*vctKey2[0] == *dvKey);

  VectorDataValue vctVal2;
  lr->GetListValue(vctVal2);
  BOOST_TEST(1 == vctVal2.size());
  BOOST_TEST(*dvVal == *vctVal[0]);

  Byte buff[100] = {};
  int pos = 0;
  utils::UInt16ToBytes((uint16_t)(15 + dvKey->GetPersistenceLength(true) +
    dvVal->GetPersistenceLength(false)), buff + pos);
  pos += 2;
  utils::UInt16ToBytes(dvKey->GetPersistenceLength(true), buff + pos);
  pos += 2;

  pos += dvKey->WriteData(buff + pos, true);
  buff[pos] = 1;
  pos++;
  utils::UInt64ToBytes(100, buff + pos);
  pos += 8;
  utils::UInt16ToBytes(9, buff + pos);
  pos += 2;

  pos += dvVal->WriteData(buff + pos, false);
  lr->ReleaseRecord();

  lr = new LeafRecord(indexTree, buff);
  BOOST_TEST(buff == lr->GetBysValue());
  BOOST_TEST(8 == lr->GetKeyLength());
  BOOST_TEST(20 == lr->GetValueLength());
  BOOST_TEST(32 == lr->GetTotalLength());

  valStru = lr->GetPriValStruct();
  BOOST_TEST(valStru->bLastOvf == false);
  BOOST_TEST(valStru->bOvf == false);
  BOOST_TEST(valStru->verCount == 1);
  BOOST_TEST(valStru->pageValOffset == 23);
  BOOST_TEST(valStru->arrStamp[0] == 100);
  BOOST_TEST(valStru->arrPageLen[0] == 9);

  VectorDataValue vctKey3;
  lr->GetListKey(vctKey3);
  BOOST_TEST(vctKey3.size() == 1);
  BOOST_TEST(*dvKey == *vctKey3[0]);

  VectorDataValue vctVal3;
  lr->GetListValue(vctVal3);
  BOOST_TEST(1 == vctVal3.size());
  BOOST_TEST(*dvVal == *vctVal3[0]);

  lr->ReleaseRecord();
  indexTree->Close(true);
  delete dvKey;
  delete dvVal;

  fs::remove(fs::path(FILE_NAME));
}

BOOST_AUTO_TEST_CASE(LeafRecord_Equal_test)
{
  const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  DataValueLong* dvKey = new DataValueLong(100, true);
  DataValueLong* dvVal = new DataValueLong(200, false);
  VectorDataValue vctKey = { dvKey->CloneDataValue(false) };
  VectorDataValue vctVal = { dvVal->CloneDataValue(false) };

  IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
  LeafPage* lp = (LeafPage*)indexTree->AllocateNewPage(HeadPage::NO_PARENT_POINTER, (Byte)0);

  Byte buff1[100] = { 0 };
  uint32_t pos = 0;
  utils::UInt16ToBytes(15 + dvKey->GetPersistenceLength(true) +
    dvVal->GetPersistenceLength(false), buff1 + pos);
  pos += 2;
  utils::UInt16ToBytes((uint16_t)dvKey->GetPersistenceLength(true), buff1 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff1 + pos, true);

  buff1[pos] = 1;
  pos++;
  utils::UInt64ToBytes(100, buff1 + pos);
  pos += 8;
  utils::UInt16ToBytes(9, buff1 + pos);
  pos += 2;

  pos += dvVal->WriteData(buff1 + pos, false);

  Byte buff2[100] = { 0 };
  pos = 0;
  utils::UInt16ToBytes(15 + dvKey->GetPersistenceLength(true) +
    dvVal->GetPersistenceLength(false), buff2 + pos);
  pos += 2;
  utils::UInt16ToBytes(dvKey->GetPersistenceLength(true), buff2 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff2 + pos, true);

  buff2[pos] = 1;
  pos++;
  utils::UInt64ToBytes(100, buff2 + pos);
  pos += 8;
  utils::UInt16ToBytes(9, buff2 + pos);
  pos += 2;

  pos += dvVal->WriteData(buff2 + pos, false);

  LeafRecord* rr1 = new LeafRecord(lp, buff1);
  LeafRecord* rr2 = new LeafRecord(lp, buff2);

  BOOST_TEST(rr1->CompareTo(*rr2) == 0);
  BOOST_TEST(rr1->CompareKey(*rr2) == 0);
  RawKey* key = rr2->GetKey();
  BOOST_TEST(rr1->CompareKey(*key) == 0);
  delete key;

  delete dvVal;
  dvVal = new DataValueLong(210, false);
  pos = 0;
  utils::UInt16ToBytes(15 + dvKey->GetPersistenceLength(true) +
    dvVal->GetPersistenceLength(false), buff2 + pos);
  pos += 2;
  utils::UInt16ToBytes((uint16_t)dvKey->GetPersistenceLength(true), buff2 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff2 + pos, true);

  buff2[pos] = 1;
  pos++;
  utils::UInt64ToBytes(100, buff2 + pos);
  pos += 8;
  utils::UInt16ToBytes(9, buff2 + pos);
  pos += 2;

  pos += dvVal->WriteData(buff2 + pos, false);

  rr2->ReleaseRecord();
  rr2 = new LeafRecord(lp, buff2);

  BOOST_TEST(rr1->CompareTo(*rr2) != 0);
  BOOST_TEST(rr1->CompareKey(*rr2) == 0);
  key = rr2->GetKey();
  BOOST_TEST(rr1->CompareKey(*key) == 0);
  delete key;
  BOOST_TEST(0 > rr1->CompareTo(*rr2));

  delete dvKey;
  dvKey = new DataValueLong(110, true);
  pos = 0;
  utils::UInt16ToBytes(15 + dvKey->GetPersistenceLength(true) +
    dvVal->GetPersistenceLength(false), buff2 + pos);
  pos += 2;
  utils::UInt16ToBytes((uint16_t)dvKey->GetPersistenceLength(true), buff2 + pos);
  pos += 2;

  pos += dvKey->WriteData(buff2 + pos, true);

  buff2[pos] = 1;
  pos++;
  utils::UInt64ToBytes(100, buff2 + pos);
  pos += 8;
  utils::UInt16ToBytes(9, buff2 + pos);
  pos += 2;

  pos += dvVal->WriteData(buff2 + pos, false);

  rr2->ReleaseRecord();
  rr2 = new LeafRecord(lp, buff2);

  BOOST_TEST(rr1->CompareKey(*rr2) != 0);
  BOOST_TEST(0 > rr1->CompareTo(*rr2));
  key = rr2->GetKey();
  BOOST_TEST(0 > rr1->CompareKey(*key));

  vctKey.push_back(dvKey->CloneDataValue(true));
  RawKey* key2 = new RawKey(vctKey);
  BOOST_TEST(0 == key2->CompareTo(*key));

  delete key2;
  delete dvKey;
  delete dvVal;
  lp->DecRefCount();
  indexTree->Close(true);
  fs::remove(fs::path(FILE_NAME));
}

BOOST_AUTO_TEST_CASE(LeafRecord_Block_test)
{
  const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  char* blockData = new char[1024 * 10];
  for (int i = 0; i < 1024 * 10; i++) {
    blockData[i] = (Byte)i;
  }

  char* blockData2 = new char[1024 * 20];
  for (int i = 0; i < 1024 * 20; i++) {
    blockData2[i] = (Byte)i;
  }

  DataValueLong* dvKey = new DataValueLong(100, true);
  DataValueLong* dvVal1 = new DataValueLong(200, false);
  DataValueBlob* dvVal2 = new DataValueBlob(1024 * 20, false);
  VectorDataValue vctKey = { dvKey->CloneDataValue(false) };
  VectorDataValue vctVal = { dvVal1->CloneDataValue(false), dvVal2->CloneDataValue(false) };

  IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);

  vctKey.push_back(dvKey->CloneDataValue(true));
  vctVal.push_back(dvVal1->CloneDataValue(true));
  vctVal.push_back(new DataValueBlob(blockData, 1024 * 10, 1024 * 20, false));

  LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal, 1);

  PageFile* ovf = indexTree->GetOverflowFile();
  uint64_t lenOvf = ovf->Length();

  LeafRecord* rr2 = new LeafRecord(indexTree, vctKey, vctVal, 2);
  uint64_t lenOvf2 = ovf->Length();
  BOOST_TEST(lenOvf <= lenOvf2);

  LeafRecord* rr3 = new LeafRecord(indexTree, vctKey, vctVal, 3, ActionType::UNKNOWN, 0, rr2);
  uint64_t lenOvf3 = ovf->Length();
  BOOST_TEST(lenOvf2 == lenOvf3);

  delete vctVal[1];
  vctVal[1] = new DataValueBlob(blockData2, 1024 * 20, 1024 * 20, false);
  LeafRecord* rr4 = new LeafRecord(indexTree, vctKey, vctVal, 4, ActionType::UNKNOWN, 0, rr);
  uint64_t lenOvf4 = ovf->Length();
  BOOST_TEST(lenOvf3 < lenOvf4);

  delete dvKey;
  delete dvVal1;
  delete dvVal2;
  rr->ReleaseRecord();
  rr2->ReleaseRecord();
  rr3->ReleaseRecord();
  rr4->ReleaseRecord();
  indexTree->Close(true);

  fs::remove(fs::path(FILE_NAME));
  string name = FILE_NAME.substr(0, FILE_NAME.find_last_of('/')) + "/overfile.dat";
  fs::remove(fs::path(name));
}

BOOST_AUTO_TEST_CASE(LeafRecord_Snapshot_test)
{
  const string FILE_NAME = "./dbTest/testLeafRecord" + utils::StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";
  char* blockData1 = new char[1024 * 2];
  for (int i = 0; i < 1024 * 2; i++) {
    blockData1[i] = (Byte)i;
  }

  char* blockData2 = new char[1024 * 5];
  for (int i = 0; i < 1024 * 5; i++) {
    blockData2[i] = (Byte)i;
  }

  char* blockData3 = new char[1024 * 10];
  for (int i = 0; i < 1024 * 10; i++) {
    blockData3[i] = (Byte)i;
  }

  DataValueLong* dvKey = new DataValueLong(100, true);
  DataValueLong* dvVal1 = new DataValueLong(200, false);
  DataValueBlob* dvVal2 = new DataValueBlob(1024 * 20, false);
  VectorDataValue vctKey = { dvKey->CloneDataValue(false) };
  VectorDataValue vctVal = { dvVal1->CloneDataValue(false), dvVal2->CloneDataValue(false) };

  IndexTree* indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal);
  indexTree->GetHeadPage()->WriteIndexType(IndexType::PRIMARY);
  indexTree->GetHeadPage()->AddNewRecordVersion(100);

  vctKey.push_back(dvKey->CloneDataValue(true));
  vctVal.push_back(dvVal1->CloneDataValue(true));
  vctVal.push_back(new DataValueBlob(blockData1, 1024 * 2, 1024 * 2, false));

  LeafRecord* rr = new LeafRecord(indexTree, vctKey, vctVal, 1);
  LeafRecord* rr2 = new LeafRecord(indexTree, vctKey, vctVal, 200, ActionType::UNKNOWN, 0, rr);

  PriValStruct* valStru = rr2->GetPriValStruct();
  BOOST_TEST(valStru->bLastOvf == false);
  BOOST_TEST(valStru->bOvf == false);
  BOOST_TEST(valStru->verCount == 2);
  BOOST_TEST(valStru->pageValOffset == 33);
  BOOST_TEST(valStru->arrStamp[0] == 200);
  BOOST_TEST(valStru->arrStamp[1] == 1);
  BOOST_TEST(valStru->arrPageLen[0] == 2062);
  BOOST_TEST(valStru->arrPageLen[1] == 4124);

  indexTree->GetHeadPage()->AddNewRecordVersion(250);
  delete vctVal[1];
  vctVal[1] = new DataValueBlob(blockData2, 1024 * 5, 1024 * 5, false);
  LeafRecord* rr3 = new LeafRecord(indexTree, vctKey, vctVal, 300, ActionType::UNKNOWN, 0, rr2);

  valStru = rr3->GetPriValStruct();
  BOOST_TEST(valStru->bLastOvf == false);
  BOOST_TEST(valStru->bOvf == true);
  BOOST_TEST(valStru->verCount == 3);
  BOOST_TEST(valStru->pageValOffset == 65);
  BOOST_TEST(valStru->arrStamp[0] == 300);
  BOOST_TEST(valStru->arrStamp[1] == 200);
  BOOST_TEST(valStru->arrStamp[2] == 1);
  BOOST_TEST(valStru->ovfOffset == 0);
  BOOST_TEST(valStru->ovfRange == 8192);
  BOOST_TEST(valStru->indexOvfStart == 2);
  BOOST_TEST(valStru->lenInPage == 5134);
  BOOST_TEST(valStru->arrOvfLen[0] == 0);
  BOOST_TEST(valStru->arrOvfLen[1] == 2062);
  BOOST_TEST(valStru->arrOvfLen[2] == 4124);

  delete vctVal[1];
  vctVal[1] = new DataValueBlob(blockData3, 1024 * 10, 1024 * 10, false);
  LeafRecord* rr4 = new LeafRecord(indexTree, vctKey, vctVal, 400, ActionType::UNKNOWN, 0, rr3);

  valStru = new PriValStruct(rr4->GetBysValue());
  BOOST_TEST(valStru->bLastOvf == true);
  BOOST_TEST(valStru->bOvf == true);
  BOOST_TEST(valStru->verCount == 3);
  BOOST_TEST(valStru->pageValOffset == 65);
  BOOST_TEST(valStru->arrStamp[0] == 400);
  BOOST_TEST(valStru->arrStamp[1] == 200);
  BOOST_TEST(valStru->arrStamp[2] == 1);
  BOOST_TEST(valStru->ovfOffset == 8192);
  BOOST_TEST(valStru->ovfRange == 16384);
  BOOST_TEST(valStru->indexOvfStart == 1);
  BOOST_TEST(valStru->lenInPage == 9);
  BOOST_TEST(valStru->arrOvfLen[0] == 10245);
  BOOST_TEST(valStru->arrOvfLen[1] == 12307);
  BOOST_TEST(valStru->arrOvfLen[2] == 14369);

  VectorDataValue vct;
  rr4->GetListKey(vct);
  BOOST_TEST(*dvKey == *vct[0]);

  vct.RemoveAll();
  rr4->GetListValue(vct, 100);
  BOOST_TEST(*dvVal1 == *vct[0]);
  DataValueBlob* dvBlob = (DataValueBlob*)vct[1];
  BOOST_TEST(2048 == dvBlob->GetDataLength());
  const char* p = (const char*)(*dvBlob);
  for (int i = 0; i < 2048; i++) {
    assert((char)i == p[i]);
  }

  vct.RemoveAll();
  rr4->GetListValue(vct);
  rr4->GetListOverflow(vct);
  BOOST_TEST(*dvVal1 == *vct[0]);
  dvBlob = (DataValueBlob*)vct[1];
  BOOST_TEST(10240 == dvBlob->GetDataLength());
  p = (const char*)(*dvBlob);
  for (int i = 0; i < 10240; i++) {
    assert((char)i == p[i]);
  }

  delete dvKey;
  delete dvVal1;
  delete dvVal2;
  rr->ReleaseRecord();
  rr2->ReleaseRecord();
  rr3->ReleaseRecord();
  rr4->ReleaseRecord();
  indexTree->Close(true);

  fs::remove(fs::path(FILE_NAME));
  string name = FILE_NAME.substr(0, FILE_NAME.find_last_of('/')) + "/overfile.dat";
  fs::remove(fs::path(name));
}

BOOST_AUTO_TEST_SUITE_END()
}
