#include "../src/core/IndexTree.h"
#include "../src/core/LeafPage.h"
#include "../src/dataType/DataValueDigit.h"
#include "../src/dataType/DataValueVarChar.h"
#include "../src/pool/CachePagePool.h"
#include "../src/pool/FilePagePool.h"
#include "../src/utils/BytesFuncs.h"
#include "../src/utils/Utilitys.h"
#include "PressTest.h"
#include <filesystem>

namespace storage {
thread_local string varchar50 = "VARCHAR_50_" + string(40, 'a');

inline uint32_t GenTestKey2(uint32_t num) {
  uint32_t by1 = num & 0xff;
  uint32_t by2 = (num >> 8) & 0xff;
  uint32_t by3 = (num >> 16) & 0xff;
  uint32_t by4 = (num >> 24) & 0xff;
  return ((by1 & 0x05) + (by2 & 0x0A) + (by3 & 0x50) + (by4 & 0xA0)) +
         (((by1 & 0xA0) + (by2 & 0x05) + (by3 & 0x0A) + (by4 & 0x50)) << 8) +
         (((by1 & 0x50) + (by2 & 0xA0) + (by3 & 0x05) + (by4 & 0x0A)) << 16) +
         (((by1 & 0x0A) + (by2 & 0x50) + (by3 & 0xA0) + (by4 & 0x05)) << 24);
}

int64_t GenPrimaryKey2(uint32_t num) {
  uint64_t val = GenTestKey2(num);
  return val * val + val;
}

VectorDataValue GenRow2(uint32_t num) {
  VectorDataValue vctDv;
  uint32_t val = GenTestKey2(num);
  DataValueLong *dvLong = new DataValueLong((int64_t)val * val + val);
  DataValueInt *dvInt = new DataValueInt(BytesSwap32(val));

  sprintf(varchar50.data() + 30, "0x%08X", (val / 10));
  const char *p = varchar50.c_str();
  DataValueVarChar *dvVar = new DataValueVarChar(p, strlen(p), 50);

  vctDv.push_back(dvLong);
  vctDv.push_back(dvInt);
  vctDv.push_back(dvVar);
  return vctDv;
}

void MultiThreadWorkProc(int threadSn, uint32_t row_count) {
  const MString FILE_NAME = "./dbTest/testMTIP" + ToMString(threadSn) + ".dat";
  const MString TABLE_NAME = "testTable";
  ThreadPool::SetThreadId(threadSn);

  MTreeMap<uint64_t, CachePage *> pageMap;
  VectorDataValue vctKey = {new DataValueLong(100)};
  VectorDataValue vctVal = {new DataValueLong(100), new DataValueInt(100),
                            new DataValueVarChar(50)};
  IndexTree *indexTree = new IndexTree();
  bool rt = indexTree->CreateIndexTree(TABLE_NAME.c_str(), TABLE_NAME.c_str(),
                                       FILE_NAME.c_str(), vctKey, vctVal,
                                       threadSn + 1, IndexType::PRIMARY);
  if (!rt) {
    abort();
  }

  chrono::system_clock::time_point st = chrono::system_clock::now();

  for (uint32_t i = 0; i < row_count; i++) {
    VectorDataValue vctVal = GenRow2(i);
    VectorDataValue vctKey = {vctVal[0]->AddRef()};
    LeafRecord *rr = new LeafRecord(indexTree, vctKey, vctVal, i, nullptr);
    IndexPage *idxPage = indexTree->GetRootPage();
    bool b = indexTree->SearchPage(*rr, idxPage);
    assert(b);

    LeafPage *lp = (LeafPage *)idxPage;
    bool bFind;
    int32_t pos = lp->SearchRecord(*rr, bFind);
    assert(!bFind && pos >= 0);
    lp->InsertRecord(rr, pos);
    lp->AddWriteQueue(pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(pageMap);
    }
  }
  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << threadSn << ":  Insert records Time(ms):" << duration.count()
           << "  Total Records: " << row_count;

  indexTree->SettleUpdatedPages(pageMap);
  assert(pageMap.size() == 0);
  indexTree->Close();
}

void MultiThreadInsertTest(int threadNum, int rowNum) {
  FilePagePool::Start(threadNum);
  thread *arrThd[threadNum];
  chrono::system_clock::time_point st = chrono::system_clock::now();
  for (int i = 0; i < threadNum; i++) {
    arrThd[i] = new thread([i, rowNum]() { MultiThreadWorkProc(i, rowNum); });
  }

  for (int i = 0; i < threadNum; i++) {
    arrThd[i]->join();
    delete arrThd[i];
  }
  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << "Insert records Time(ms):" << duration.count()
           << "  Total Records: " << rowNum * threadNum;

  CachePagePool::ClearPool();
  FilePagePool::Stop();
}

} // namespace storage
