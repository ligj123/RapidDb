#include "PressTest.h"

#include "../src/binlog/LogTask.h"
#include "../src/cache/Mallocator.h"
#include "../src/core/BranchPage.h"
#include "../src/core/IndexTree.h"
#include "../src/core/LeafPage.h"
#include "../src/core/LeafRecord.h"
#include "../src/manager/DatabaseManager.h"
#include "../src/manager/TableManager.h"
#include "../src/pool/CachePagePool.h"
#include "../src/pool/FilePagePool.h"
#include "../src/serv/Session.h"
#include "../src/serv/SessionPool.h"
#include "../src/statement/StmtResult.h"
#include "../src/table/Table.h"
#include "../src/table/TableTaskMgr.h"

namespace storage {
const char *ROOT_PATH = "./TestPress";
const char *DB_NAME = "dbTest";
const char *TBL_NAME = "tableTest";
const char *DB_TBL_NAME = "dbTest.tableTest";
const char *INSERT_STMT = "insert into dbTest.tableTest values(?, ?, ?)";
const char *UPDATE_STMT = "update dbTest.tableTest set c2=c2+1 where c1=?";
const char *UPDATE_STMT2 = "update dbTest.tableTest set c2=c2-1 where c1=?";
const char *DELETE_STMT = "delete from dbTest.tableTest where c1=?";
const char *SELECT_STMT = "select * from dbTest.tableTest where c1=?";
thread_local string varchar = "VARCHAR_50_" + string(40, 'a');

PhysTable *table = nullptr;
const int MAX_USER_THREADS = 8;
Byte *arrResult = nullptr;
int *arrNum = nullptr;
// The redio of insert, update , delete, select
enum class OpRedio : uint8_t {
  INS, // Insert
  UPD, // Update
  DEL, // Delete
  SEL  // Select
};

OpRedio arrRadio[] = {OpRedio::INS, OpRedio::UPD, OpRedio::DEL, OpRedio::SEL,
                      OpRedio::SEL, OpRedio::SEL, OpRedio::SEL, OpRedio::SEL,
                      OpRedio::SEL, OpRedio::SEL};
int redioCount = sizeof(arrRadio);

struct ResultStat {
  int _insertPassed{0};
  int _insertFailed{0};
  int _deletePassed{0};
  int _deleteFailed{0};
  int _updatePassed{0};
  int _updateFailed{0};
  int _selectPassed{0};
  int _selectFailed{0};
};
ResultStat arrResStat[MAX_USER_THREADS];

inline uint32_t GenTestKey(uint32_t num) {
  uint32_t by1 = num & 0xff;
  uint32_t by2 = (num >> 8) & 0xff;
  uint32_t by3 = (num >> 16) & 0xff;
  uint32_t by4 = (num >> 24) & 0xff;
  return ((by1 & 0x05) + (by2 & 0x0A) + (by3 & 0x50) + (by4 & 0xA0)) +
         (((by1 & 0xA0) + (by2 & 0x05) + (by3 & 0x0A) + (by4 & 0x50)) << 8) +
         (((by1 & 0x50) + (by2 & 0xA0) + (by3 & 0x05) + (by4 & 0x0A)) << 16) +
         (((by1 & 0x0A) + (by2 & 0x50) + (by3 & 0xA0) + (by4 & 0x05)) << 24);
}

int64_t GenPrimaryKey(uint32_t num) {
  uint64_t val = GenTestKey(num);
  return val * val + val;
}

VectorRow GenRow(uint32_t num) {
  VectorDataValue vctDv;
  uint32_t val = GenTestKey(num);
  DataValueLong *dvLong = new DataValueLong((int64_t)val * val + val);
  DataValueInt *dvInt = new DataValueInt(BytesSwap32(val));

  sprintf(varchar.data() + 30, "0x%08X", (val / 10));
  const char *p = varchar.c_str();
  DataValueVarChar *dvVar = new DataValueVarChar(p, strlen(p), 50);

  vctDv.push_back(dvLong);
  vctDv.push_back(dvInt);
  vctDv.push_back(dvVar);
  VectorRow vctRow;
  vctRow.push_back(move(vctDv));

  return vctRow;
}

void CreateDbTable(bool bExclusive) {
  Database *db =
      new Database(1, ROOT_PATH, DB_NAME, MilliSecTime(), MicroSecTime());
  DatabaseManager::AddDb(db);

  PhysTable *ptable =
      new PhysTable(db, TBL_NAME, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::LONG, false, -1, "primary key",
                    Charsets::UNKNOWN, nullptr);
  ptable->AddColumn("c2", DataType::INT, false, -1, "Unique Key",
                    Charsets::UNKNOWN, nullptr);
  ptable->AddColumn("c3", DataType::VARCHAR, true, 50, "NonUnique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});

  bool b = ptable->OpenIndex(0, true);
  assert(b);

  TableTaskMgr *tmgr =
      new TableTaskMgr(ThreadPool::GetMainPool(), ptable, 1, bExclusive);
  ptable->SetTableTaskMgr(tmgr);
  TableManager::AddTable(DB_TBL_NAME, ptable);
  table = ptable;
}

void CheckSelectResult(uint32_t num, VectorDataValue &vctDv) {
  uint32_t val = GenTestKey(num);
  int64_t pkval = GenPrimaryKey(num);

  if (vctDv[0]->GetLong() != pkval) {
    LOG_ERROR << num << "  Error first field value, expect value: " << pkval
              << "  actual value: " << vctDv[0]->GetLong();
  }

  int secVal = (int32_t)BytesSwap32(val) + (arrResult[num] & 0x7f);
  if (vctDv[1]->GetLong() != secVal) {
    LOG_ERROR << num
              << "  Error secondary field value, expect value: " << secVal
              << "  actual value: " << vctDv[1]->GetLong();
  }

  sprintf(varchar.data() + 30, "0x%08X", (val / 10));
  const Byte *p = (const Byte *)varchar.c_str();
  if (BytesCompare(p, strlen(varchar.c_str()) + 1, vctDv[2]->GetBuff(),
                   vctDv[2]->GetDataLength()) != 0) {
    LOG_ERROR << num
              << "  Error third field value, expect value: " << varchar.c_str()
              << "  actual value: " << (const char *)vctDv[2]->GetBuff();
  }
}

void GetRecordValue(int num) {
  int64_t val = GenTestKey(num);
  DataValueLong *dvLong = new DataValueLong(val * val + val);
  RawKey key({dvLong});

  IndexTree *idxTree = table->GetVectorIndex()[0]._tree;
  IndexPage *idxPage = idxTree->GetRootPage();
  bool b = idxTree->SearchPage(key, idxPage);
  assert(b);

  LeafPage *lpage = dynamic_cast<LeafPage *>(idxPage);
  bool bFind;
  int pos = lpage->SearchKey(key, bFind);
  if (!bFind) {
    LOG_INFO << "Failed to find num: " << num;
    return;
  }

  LeafRecord &lr = lpage->GetRecord(pos);
  VectorDataValue vctDv;
  ReadResult rr = lr.ReadListValue({}, vctDv, idxTree);
  if (rr == ReadResult::REC_DELETE) {
    LOG_INFO << "Delete Num: " << num;
    return;
  }
  LOG_INFO << "C1: " << vctDv[0]->GetLong() << "\tC2: " << vctDv[1]->GetLong()
           << "\tc3: " << (const char *)vctDv[2]->GetBuff();
}

void PrintNum(int rowNum, int num) {
  for (int i = 0; i < rowNum; i++) {
    if (arrNum[i] == num) {
      LOG_INFO << i;
    }
  }
}

void CheckAllRecord(int rowNum) {
  IndexTree *idxTree = table->GetVectorIndex()[0]._tree;
  LeafPage *lpage = idxTree->GetBeginPage();

  MTreeMap<int64_t, int> map;
  for (int i = 0; i < rowNum; i++) {
    map.emplace(GenPrimaryKey(i), i);
  }

  int cnt = 0;
  auto iter = map.begin();
  auto itOld = iter;

  while (lpage != nullptr) {
    for (int i = 0; i < lpage->GetRecordNumber(); i++) {
      LeafRecord &lr = lpage->GetRecord(i);
      if (lr.ReleaseLockAble()) {
        lr.ReleaseLock(idxTree);
      }

      while (true) {
        RawKey key({new DataValueLong(iter->first)});
        int hr = lr.CompareKey(key);
        assert(hr >= 0);
        if (hr == 0)
          break;

        iter++;
        cnt++;
      }

      VectorDataValue vctDv;
      ReadResult rr = lr.ReadListValue({}, vctDv, idxTree);
      if (rr == ReadResult::REC_DELETE) {
        assert((arrResult[iter->second] & 0x80) == 0);
      } else {
        CheckSelectResult(iter->second, vctDv);
      }
      cnt++;
      itOld = iter;
      iter++;
    }

    lpage = lpage->GetNextPage();
  }

  LOG_INFO << "cnt: " << cnt;
}

void InsertProc(uint16_t tid, MVector<uint32_t> vctSessId, int recStart,
                int recNum) {
  MVector<StmtResult> vctResult(vctSessId.size());
  MVector<int> vctStmtId(vctSessId.size());

  int cnt = 0;
  while (true) {
    int unfinished = 0;
    for (size_t i = 0; i < vctResult.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs == ResultStatus::FILLING) {
        unfinished++;
        continue;
      }

      if (rs == ResultStatus::FINISHED) {
        assert(vctResult[i]._rowNum == 1 && vctResult[i]._vctError.size() == 0);
        assert(vctResult[i]._stmtId == vctStmtId[i]);
      }

      if (cnt < recNum) {
        arrResult[recStart + cnt] = 0x80;
        VectorRow vctRow = GenRow(recStart + cnt);
        SessionPool::AddStatement(tid, vctSessId[i], cnt, 1, INSERT_STMT,
                                  move(vctRow), &vctResult[i]);
        vctStmtId[i] = cnt;
        unfinished++;
        cnt++;
      }
    }

    if (unfinished == 0) {
      break;
    }
  }
}

void StatementProc(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                   int recNum, int opTimes) {
  assert(tid < MAX_USER_THREADS);
  MVector<StmtResult> vctResult(vctSessId.size());
  // Response for vctResult one by one, pair<the int value, which operation>
  MVector<pair<int, OpRedio>> vctPair(vctSessId.size());
  for (size_t i = 0; i < vctPair.size(); i++) {
    vctPair[i].first = -1;
  }

  srand(tid);
  int cnt = 0;

  while (true) {
    int empty = 0;
    for (size_t i = 0; i < vctSessId.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs != ResultStatus::FINISHED) {
        continue;
      }

      pair<int, OpRedio> &pr = vctPair[i];
      if (pr.first < 0) {
        empty++;
        continue;
      }

      arrResult[pr.first] &= 0xBF;
      StmtResult &rst = vctResult[i];

      switch (pr.second) {
      case OpRedio::INS:
        if (!rst._bFailed) {
          assert(arrResult[pr.first] == 0);
          arrResStat[tid]._insertPassed++;
          arrResult[pr.first] = 0x80;
        } else {
          assert(arrResult[pr.first] != 0);
          arrResStat[tid]._insertFailed++;
        }
        break;
      case OpRedio::UPD:
        assert(!rst._bFailed);
        if (rst._rowNum > 0) {
          assert(arrResult[pr.first] >= 0x80);
          arrResStat[tid]._updatePassed++;
        } else {
          assert(arrResult[pr.first] == 0);
          arrResStat[tid]._updateFailed++;
        }
        break;
      case OpRedio::DEL:
        assert(!rst._bFailed);
        if (rst._rowNum > 0) {
          assert(arrResult[pr.first] > 0);
          arrResStat[tid]._deletePassed++;
          arrResult[pr.first] = 0;
        } else {
          assert(arrResult[pr.first] == 0);
          arrResStat[tid]._deleteFailed++;
        }
        break;
      case OpRedio::SEL:
        assert(!rst._bFailed);
        if (rst._rowNum > 0) {
          assert(arrResult[pr.first] > 0);
          arrResStat[tid]._selectPassed++;
          rst._resultSet->First();

          VectorDataValue vctDv;
          rst._resultSet->GetCurrDataValueRow(vctDv);
          CheckSelectResult(pr.first, vctDv);
        } else {
          assert(arrResult[pr.first] == 0);
          arrResStat[tid]._selectFailed++;
        }
        break;
      default:
        abort();
      }

      pr.first = -1;
    }

    if (cnt >= opTimes) {
      if (empty == vctSessId.size()) {
        break;
      }

      continue;
    }

    for (size_t i = 0; i < vctSessId.size(); i++) {
      pair<int, OpRedio> &pr = vctPair[i];
      if (pr.first >= 0) {
        continue;
      }

      int currVal;
      int tryTime = 0;
      while (true) {
        currVal = cnt % recNum + rand() % 100 - 50;
        if (currVal >= recNum) {
          currVal -= 50;
        } else if (currVal < 0) {
          currVal += 50;
        }

        currVal += startRec;
        if ((arrResult[currVal] & 0x40) == 0) {
          break;
        }
        if (++tryTime > 50) {
          cnt--;
          continue;
        }
      }

      arrNum[cnt] = currVal;
      OpRedio redio = arrRadio[cnt % redioCount];
      pr.first = currVal;
      pr.second = redio;
      arrResult[currVal] |= 0x40;
      VectorRow vctRow;

      switch (redio) {
      case OpRedio::INS: {
        vctRow = GenRow(currVal);
        vctResult[i]._rowNum = arrResult[currVal] & 0xBF;
        SessionPool::AddStatement(tid, vctSessId[i], cnt, 1, INSERT_STMT,
                                  move(vctRow), &vctResult[i]);
        break;
      }
      case OpRedio::UPD: {
        vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
        if (arrResult[currVal] & 0x80) {
          if ((arrResult[currVal] & 0x3F) != 0x3F) {
            arrResult[currVal] += 1;
            SessionPool::AddStatement(tid, vctSessId[i], cnt, 2, UPDATE_STMT,
                                      move(vctRow), &vctResult[i]);
          } else {
            arrResult[currVal] -= 1;
            SessionPool::AddStatement(tid, vctSessId[i], cnt, 5, UPDATE_STMT2,
                                      move(vctRow), &vctResult[i]);
          }
        } else {
          SessionPool::AddStatement(tid, vctSessId[i], cnt, 2, UPDATE_STMT,
                                    move(vctRow), &vctResult[i]);
        }
        break;
      }
      case OpRedio::DEL: {
        vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
        SessionPool::AddStatement(tid, vctSessId[i], cnt, 3, DELETE_STMT,
                                  move(vctRow), &vctResult[i]);
        break;
      }
      case OpRedio::SEL: {
        vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
        SessionPool::AddStatement(tid, vctSessId[i], cnt, 4, SELECT_STMT,
                                  move(vctRow), &vctResult[i]);
        break;
      }
      default:
        abort();
      }

      cnt++;
    }
  }
}

void TablePointTest(uint16_t userThreads, uint16_t poolThreads,
                    uint16_t sessGroupNum, uint16_t sessTaskNum, int sessionNum,
                    int rowNum, int totalOpTimes, bool bExclusive) {
  assert(sessionNum % userThreads == 0);
  ThreadPool *tpool = ThreadPool::CreateMainPool("press", 1, poolThreads);
  ThreadPool::SetThreadId(0);
  FilePagePool::Start(poolThreads);
  LogTask::InitLogTask(tpool, "./binlog/", bExclusive);
  SessionPool::InitPool(sessGroupNum, sessTaskNum, 0, userThreads, tpool,
                        bExclusive);
  // CachePagePoolTask::Init(tpool);

  arrResult = new Byte[rowNum];
  memset(arrResult, 0, rowNum);
  arrNum = new int[totalOpTimes];
  memset(arrNum, 0, totalOpTimes * 4);

  CreateDbTable(bExclusive);
  vector<uint32_t> vctSessId;
  vector<StmtResult> vctStmtRes(sessionNum);

  for (int i = 0; i < sessionNum; i++) {
    uint32_t sid = SessionPool::CreateSession(0, &vctStmtRes[i]);
    vctSessId.push_back(sid);
  }

  for (int i = 0; i < sessionNum; i++) {
    while (vctStmtRes[i].GetResultStatus() != ResultStatus::FINISHED) {
      this_thread::yield();
    }
  }

  vector<thread *> vctThread;
  vctThread.reserve(userThreads);
  int sRange = sessionNum / userThreads;
  int rRange = rowNum / userThreads;
  ThreadPool::PrintThreadTime();
  chrono::system_clock::time_point st = chrono::system_clock::now();

  for (int i = 0; i < userThreads; i++) {
    MVector<uint32_t> vct;
    vct.insert(vct.end(), vctSessId.begin() + i * sRange,
               vctSessId.begin() + (i + 1) * sRange);
    int recStart = i * rRange;
    thread *t = new thread(
        [i, vct, recStart, rRange]() { InsertProc(i, vct, recStart, rRange); });
    vctThread.push_back(t);
  }

  for (int i = 0; i < userThreads; i++) {
    vctThread[i]->join();
    delete vctThread[i];
  }

  vctThread.clear();
  chrono::system_clock::time_point et = chrono::system_clock::now();
  ThreadPool::PrintThreadTime();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << "Insert records Time(ms):" << duration.count()
           << "  Total Records: " << rowNum;

  IndexPage *rootPage = table->GetVectorIndex()[0]._tree->GetRootPage();
  LOG_INFO << "Root RecordNumber: " << rootPage->GetRecordNumber()
           << "  PageLevel: " << (int)rootPage->GetPageLevel();

  TableTaskMgr::_dtLastWriteDisk = MicroSecTime();
  IndexRange &range = table->GetVectorIndex()[0]._tree->GetVctRange()[0];
  while (range._pageMap.size() > 1 || FilePagePool::IsBusy()) {
    this_thread::sleep_for(1us);
  }
  rootPage = table->GetVectorIndex()[0]._tree->GetRootPage();
  LOG_INFO << "Root RecordNumber: " << rootPage->GetRecordNumber()
           << "  PageLevel: " << (int)rootPage->GetPageLevel();

  // IndexAdjustTask *adjustTask =
  //     new IndexAdjustTask(tpool, table->GetTableTaskMgr(), 0, 2, true);
  // tpool->AddTask(adjustTask);
  ThreadPool::PrintThreadTime();
  st = chrono::system_clock::now();
  int opTimes = totalOpTimes / userThreads;
  for (int i = 0; i < userThreads; i++) {
    MVector<uint32_t> vct;
    vct.insert(vct.end(), vctSessId.begin() + i * sRange,
               vctSessId.begin() + (i + 1) * sRange);
    int recStart = i * rRange;
    thread *t = new thread([i, vct, recStart, rRange, opTimes]() {
      StatementProc(i, vct, recStart, rRange, opTimes);
    });
    vctThread.push_back(t);
  }

  for (int i = 0; i < userThreads; i++) {
    vctThread[i]->join();
    delete vctThread[i];
  }

  vctThread.clear();
  et = chrono::system_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << "Operator records Time(ms):" << duration.count()
           << "  Total times: " << totalOpTimes;
  ThreadPool::PrintThreadTime();
  CheckAllRecord(rowNum);
  PhysTable *tbl = nullptr;
  TableManager::FindTable(DB_TBL_NAME, tbl);
  tbl->GetTableTaskMgr()->SetMgrStatus(MgrStatus::SET_STOP);
  MVector<storage::IndexRange> &vctRange =
      table->GetVectorIndex()[0]._tree->GetVctRange();
  while (true) {
    bool bEmpty = true;
    for (size_t i = 0; i < vctRange.size(); i++) {
      IndexRange &range = vctRange[i];
      if (i == 0 && range._pageMap.size() > 1 ||
          i > 0 && range._pageMap.size() > 0) {
        bEmpty = false;
      }
    }

    if (bEmpty) {
      break;
    }

    this_thread::sleep_for(1us);
  }
  SessionPool::ClosePool();

  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  TableManager::ClearTable();
  DatabaseManager::ClearDB();
  SessionPool::ClearPool();
  CachePagePool::ClearPool();
  LogTask::Clear();
  _threadErrorMsg.reset();
  ErrorMsg::ClearErrorMsg();

  LOG_INFO << "Memory leaked: " << CachePool::GetMemoryUsed();
#ifdef CACHE_TRACE
  unordered_map<uint64_t, string> &map = CachePool::_mapApply;
  for (auto iter = map.begin(); iter != map.end(); iter++) {
    LOG_INFO << (void *)iter->first << "    " << iter->second;
  }
#endif // CACHE_TRACE
}

} // namespace storage