#include "../src/core/IndexTree.h"
#include "../src/dataType/DataValueDigit.h"
#include "../src/pool/PageBufferPool.h"
#include "../src/pool/PageDividePool.h"
#include "../src/pool/StoragePool.h"
#include "../src/utils/BytesConvert.h"
#include "../src/utils/Utilitys.h"
#include "PressTest.h"
#include <filesystem>

namespace storage {
void InsertSpeedPrimaryTest(uint64_t row_count) {
  const string FILE_NAME =
      "./dbTest/testInsertSpeedPrimary" + StrMSTime() + ".dat";
  const string TABLE_NAME = "testTable";

  // PageDividePool::SetThreadStatus(true);
  // StoragePool::SetWriteSuspend(true);
  if (row_count < 1000)
    row_count = 10000000;

  DataValueLong *dvKey = new DataValueLong(100, true);
  DataValueLong *dvVal = new DataValueLong(200, false);
  VectorDataValue vctKey = {dvKey->Clone()};
  VectorDataValue vctVal = {dvVal->Clone()};
  // IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey, vctVal,
  // IndexType::PRIMARY);

  vctKey.push_back(dvKey->Clone());
  vctVal.push_back(dvVal->Clone());

  uint64_t dtStart = MilliSecTime();
  uint64_t dtPrev = dtStart;

  // for (uint64_t i = 0; i < row_count; i++) {
  //   uint64_t num = i;
  //   uint64_t by1 = num & 0xff;
  //   uint64_t by2 = (num >> 8) & 0xff;
  //   uint64_t by3 = (num >> 16) & 0xff;
  //   uint64_t by4 = (num >> 24) & 0xff;
  //   uint64_t priKey = (((by1 & 0x55) + (by4 & 0xAA)) << 24) +
  //                     (((by2 & 0x55) + (by3 & 0xAA)) << 16) +
  //                     (((by2 & 0xAA) + (by3 & 0x55)) << 8) +
  //                     (((by1 & 0xAA) + (by4 & 0x55)));
  //   *((DataValueLong *)vctKey[0]) = priKey;
  //   *((DataValueLong *)vctVal[0]) = i;
  //   LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, i, nullptr);
  //   // indexTree->GetHeadPage()->ReadRecordStamp());
  //   indexTree->InsertRecord(rr);

  //  if (i % 1000000 == 0) {
  //    uint64_t dt = MilliSecTime();
  //    cout << "i=" << i << "\tTotal Time=" << (dt - dtStart)
  //         << "\tGap Time=" << (dt - dtPrev) << endl;
  //    dtPrev = dt;
  //  }
  //}

  uint64_t dtEnd = MilliSecTime();
  cout << "Insert Used Time: " << (dtEnd - dtStart) << endl;
  // PageDividePool::SetThreadStatus(false);
  // StoragePool::SetWriteSuspend(false);
  // indexTree->Close(true);
  delete dvKey;
  delete dvVal;
  // PageBufferPool::ClearPool();
  dtEnd = MilliSecTime();
  cout << "Total Used Time: " << (dtEnd - dtStart) << endl;

  // std::filesystem::remove(std::filesystem::path(FILE_NAME));
}

void InsertSpeedUniqueTest(uint64_t row_count) {
  // const string FILE_NAME =
  //     "./dbTest/testInsertSpeedUnique" + StrMSTime() + ".dat";
  // const string TABLE_NAME = "testTable";

  //// PageDividePool::SetThreadStatus(true);
  //// StoragePool::SetWriteSuspend(true);
  // if (row_count < 1000)
  //   row_count = 10000000;

  // DataValueLong *dvKey = new DataValueLong(100, true);
  // DataValueLong *dvVal = new DataValueLong(200, true);
  // VectorDataValue vctKey = {dvKey->Clone()};
  // VectorDataValue vctVal = {dvVal->Clone()};
  // IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey,
  // vctVal); indexTree->GetHeadPage()->WriteIndexType(IndexType::UNIQUE);

  // vctKey.push_back(dvKey->Clone());
  // vctVal.push_back(dvVal->Clone());

  // uint64_t dtStart = MilliSecTime();
  // uint64_t dtPrev = dtStart;
  // Byte buf[100];

  // for (uint64_t i = 0; i < row_count; i++) {
  //   uint64_t num = i;
  //   uint64_t by1 = num & 0xff;
  //   uint64_t by2 = (num >> 8) & 0xff;
  //   uint64_t by3 = (num >> 16) & 0xff;
  //   uint64_t by4 = (num >> 24) & 0xff;
  //   uint64_t priKey = (((by1 & 0x55) + (by4 & 0xAA)) << 24) +
  //                     (((by2 & 0x55) + (by3 & 0xAA)) << 16) +
  //                     (((by2 & 0xAA) + (by3 & 0x55)) << 8) +
  //                     (((by1 & 0xAA) + (by4 & 0x55)));
  //   *((DataValueLong *)vctKey[0]) = priKey;
  //   Int64ToBytes(i, buf, true);
  //   LeafRecord *rr =
  //       new LeafRecord(indexTree, vctKey, buf, 8, ActionType::INSERT,
  //       nullptr);
  //   indexTree->InsertRecord(rr);

  //  if (i % 1000000 == 0) {
  //    uint64_t dt = MilliSecTime();
  //    cout << "i=" << i << "\tTotal Time=" << (dt - dtStart)
  //         << "\tGap Time=" << (dt - dtPrev) << endl;
  //    dtPrev = dt;
  //  }
  //}

  // uint64_t dtEnd = MilliSecTime();
  // cout << "Insert Used Time: " << (dtEnd - dtStart) << endl;
  // PageDividePool::SetThreadStatus(false);
  // StoragePool::SetWriteSuspend(false);
  // indexTree->Close(true);
  // delete dvKey;
  // delete dvVal;
  //// PageBufferPool::ClearPool();
  // dtEnd = MilliSecTime();
  // cout << "Total Used Time: " << (dtEnd - dtStart) << endl;
  //// std::filesystem::remove(std::filesystem::path(FILE_NAME));
}

void InsertSpeedNonUniqueTest(uint64_t row_count) {
  // const string FILE_NAME =
  //     "./dbTest/testInsertSpeedNonUnique" + StrMSTime() + ".dat";
  // const string TABLE_NAME = "testTable";

  //// PageDividePool::SetThreadStatus(true);
  //// StoragePool::SetWriteSuspend(true);
  // if (row_count < 1000)
  //   row_count = 10000000;

  // DataValueLong *dvKey = new DataValueLong(100, true);
  // DataValueLong *dvVal = new DataValueLong(200, true);
  // VectorDataValue vctKey = {dvKey->Clone()};
  // VectorDataValue vctVal = {dvVal->Clone()};
  // IndexTree *indexTree = new IndexTree(TABLE_NAME, FILE_NAME, vctKey,
  // vctVal); indexTree->GetHeadPage()->WriteIndexType(IndexType::NON_UNIQUE);

  // vctKey.push_back(dvKey->Clone());
  // vctVal.push_back(dvVal->Clone());

  // uint64_t dtStart = MilliSecTime();
  // uint64_t dtPrev = dtStart;
  // Byte buf[100];

  // for (uint64_t i = 0; i < row_count; i++) {
  //   uint64_t num = i % (row_count / 3);
  //   uint64_t by1 = num & 0xff;
  //   uint64_t by2 = (num >> 8) & 0xff;
  //   uint64_t by3 = (num >> 16) & 0xff;
  //   uint64_t by4 = (num >> 24) & 0xff;
  //   uint64_t priKey = (((by1 & 0x55) + (by4 & 0xAA)) << 24) +
  //                     (((by2 & 0x55) + (by3 & 0xAA)) << 16) +
  //                     (((by2 & 0xAA) + (by3 & 0x55)) << 8) +
  //                     (((by1 & 0xAA) + (by4 & 0x55)));
  //   *((DataValueLong *)vctKey[0]) = priKey;
  //   Int64ToBytes(i, buf, true);
  //   LeafRecord *rr =
  //       new LeafRecord(indexTree, vctKey, buf, 8, ActionType::INSERT,
  //       nullptr);
  //   indexTree->InsertRecord(rr);

  //  if (i % 1000000 == 0) {
  //    uint64_t dt = MilliSecTime();
  //    cout << "i=" << i << "\tTotal Time=" << (dt - dtStart)
  //         << "\tGap Time=" << (dt - dtPrev) << endl;
  //    dtPrev = dt;
  //  }
  //}

  // uint64_t dtEnd = MilliSecTime();
  // cout << "Insert Used Time: " << (dtEnd - dtStart) << endl;
  // PageDividePool::SetThreadStatus(false);
  // StoragePool::SetWriteSuspend(false);
  // indexTree->Close(true);
  // delete dvKey;
  // delete dvVal;
  //// PageBufferPool::ClearPool();
  // dtEnd = MilliSecTime();
  // cout << "Total Used Time: " << (dtEnd - dtStart) << endl;
  //// std::filesystem::remove(std::filesystem::path(FILE_NAME));
}
} // namespace storage
