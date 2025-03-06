#include "PressTest.h"

#include "../src/binlog/LogTask.h"
#include "../src/cache/Mallocator.h"
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
const char *DELETE_STMT = "delete dbTest.tableTest where c1=?";
const char *SELECT_STMT = "select * from dbTest.tableTest where c1=?";
thread_local MString varchar = "VARCHAR_50_" + MString(40, 'a');

const int MAX_USER_THREADS = 8;
Byte *arrResult = nullptr;
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

inline uint32_t GenTestPrimaryKey(uint32_t num) {
  uint32_t by1 = num & 0xff;
  uint32_t by2 = (num >> 8) & 0xff;
  uint32_t by3 = (num >> 16) & 0xff;
  uint32_t by4 = (num >> 24) & 0xff;
  return ((by1 & 0x05) + (by2 & 0x0A) + (by3 & 0x50) + (by4 & 0xA0)) +
         (((by1 & 0xA0) + (by2 & 0x05) + (by3 & 0x0A) + (by4 & 0x50)) << 8) +
         (((by1 & 0x50) + (by2 & 0xA0) + (by3 & 0x05) + (by4 & 0x0A)) << 16) +
         (((by1 & 0x0A) + (by2 & 0x50) + (by3 & 0xA0) + (by4 & 0x05)) << 24);
}

VectorRow GenRow(uint32_t num) {
  VectorDataValue vctDv;
  uint32_t val = GenTestPrimaryKey(num);
  DataValueLong *dvLong = new DataValueLong((int64_t)val * val + val);
  DataValueInt *dvInt = new DataValueInt(BytesSwap32(val));

  sprintf(varchar.data() + 30, "0x%8X", (num / 10));
  const char *p = varchar.c_str();
  DataValueVarChar *dvVar = new DataValueVarChar(p, strlen(p), 50);

  vctDv.push_back(dvLong);
  vctDv.push_back(dvInt);
  vctDv.push_back(dvVar);
  VectorRow vctRow;
  vctRow.push_back(move(vctDv));

  return vctRow;
}

void CreateDbTable() {
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

  TableTaskMgr *tmgr = new TableTaskMgr(ThreadPool::GetMainPool(), ptable, 1);
  ptable->SetTableTaskMgr(tmgr);
  TableManager::AddTable(DB_TBL_NAME, ptable);
}

void InsertProc(uint16_t tid, MVector<uint32_t> vctSessId, int recStart,
                int recNum) {
  MVector<StmtResult> vctResult(vctSessId.size());

  int cnt = 0;
  while (true) {
    int unfinished = 0;
    for (size_t i = 0; i < vctResult.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs == ResultStatus::FILLING) {
        unfinished++;
        continue;
      }

      assert(rs != ResultStatus::FINISHED ||
             (vctResult[i]._rowNum == 1 && vctResult[i]._vctError.size() == 0));

      if (cnt < recNum) {
        arrResult[recStart + cnt] = 0x80;
        VectorRow vctRow = GenRow(recStart + cnt);
        SessionPool::AddStatement(tid, vctSessId[i], cnt, 1, INSERT_STMT,
                                  move(vctRow), &vctResult[i]);
        unfinished++;
        cnt++;
      }
    }

    if (unfinished == 0) {
      break;
    }
  }
}

void CheckSelectResult(int val, VectorDataValue &vctDv) {
  if (vctDv[0]->GetLong() != (int64_t)val * val + val) {
    LOG_ERROR << "Error first field value, expect value: "
              << (int64_t)val * val + val
              << "  actual value: " << vctDv[0]->GetLong();
    abort();
  }

  int secVal = (int32_t)BytesSwap32(val) + (arrResult[val] & 0x7f);
  if (vctDv[1]->GetLong() != secVal) {
    LOG_ERROR << "Error secondary field value, expect value: " << secVal
              << "  actual value: " << vctDv[1]->GetLong();
    abort();
  }

  sprintf(varchar.data() + 30, "0x%8X", (val / 10));
  const Byte *p = (const Byte *)varchar.c_str();
  if (BytesCompare(p, strlen(varchar.c_str()), vctDv[2]->GetBuff(),
                   vctDv[2]->GetDataLength()) != 0) {
    LOG_ERROR << "Error third field value, expect value: " << varchar.c_str()
              << "  actual value: " << (const char *)vctDv[2]->GetBuff();
    abort();
  }
}

void StatementProc(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                   int recNum, int opTimes) {
  assert(tid < MAX_USER_THREADS);
  MVector<StmtResult> vctResult(vctSessId.size());
  // Response for vctResult one by one, pair<the int value, which operation>
  MVector<pair<int, OpRedio>> vctPair(vctSessId.size());
  srand(tid);

  int cnt = 0;
  int pos = -1;
  int finished = 0;
  while (true) {
    pos++;
    if (pos >= vctSessId.size()) {
      pos = 0;
    }

    ResultStatus rs = vctResult[pos].GetResultStatus();
    if (rs == ResultStatus::FILLING) {
      if (cnt >= opTimes) {
        finished = 0;
      }

      continue;
    }

    if (cnt >= opTimes) {
      finished++;
      if (finished >= vctSessId.size()) {
        break;
      }
    }

    if (rs == ResultStatus::FINISHED) {
      pair<int, OpRedio> &pr = vctPair[pos];
      StmtResult &rst = vctResult[pos];
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
          assert(arrResult[pr.first] > 0);
          arrResStat[tid]._updatePassed++;
          arrResult[pr.first] += 1;
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
          vctResult[pos]._resultSet->First();

          VectorDataValue vctDv;
          vctResult[pos]._resultSet->GetCurrDataValueRow(vctDv);
          CheckSelectResult(pr.first, vctDv);
        } else {
          assert(arrResult[pr.first] == 0);
          arrResStat[tid]._selectFailed++;
        }
        break;
      default:
        abort();
      }
    }

    int currVal = cnt % recNum + rand() % 50 - 25;
    if (currVal >= recNum) {
      currVal -= 25;
    } else if (currVal < 0) {
      currVal += 25;
    }

    currVal += startRec;
    OpRedio redio = arrRadio[cnt % redioCount];
    vctPair[pos].first = currVal;
    vctPair[pos].second = redio;
    VectorRow vctRow;

    switch (redio) {
    case OpRedio::INS: {
      vctRow = GenRow(currVal);
      SessionPool::AddStatement(tid, vctSessId[pos], cnt, 1, INSERT_STMT,
                                move(vctRow), &vctResult[pos]);
      break;
    }
    case OpRedio::UPD: {
      vctRow.push_back(
          {new DataValueLong((int64_t)currVal * currVal + currVal)});
      SessionPool::AddStatement(tid, vctSessId[pos], cnt, 1, UPDATE_STMT,
                                move(vctRow), &vctResult[pos]);
      break;
    }
    case OpRedio::DEL: {
      vctRow.push_back(
          {new DataValueLong((int64_t)currVal * currVal + currVal)});
      SessionPool::AddStatement(tid, vctSessId[pos], cnt, 1, DELETE_STMT,
                                move(vctRow), &vctResult[pos]);
      break;
    }
    case OpRedio::SEL: {
      vctRow.push_back(
          {new DataValueLong((int64_t)currVal * currVal + currVal)});
      SessionPool::AddStatement(tid, vctSessId[pos], cnt, 1, SELECT_STMT,
                                move(vctRow), &vctResult[pos]);
    }
    default:
      abort();
    }

    cnt++;
  }
}

void TablePointTest(uint16_t userThreads, uint16_t poolThreads,
                    uint16_t sessGroupNum, uint16_t sessTaskNum, int sessionNum,
                    int rowNum, int totalOpTimes) {
  assert(sessionNum % userThreads == 0);
  ThreadPool *tpool = ThreadPool::CreateMainPool("press", 1, poolThreads);
  FilePagePool::Start(poolThreads);
  LogTask::InitLogTask(tpool, "./binlog/");
  SessionPool::InitPool(sessGroupNum, sessTaskNum, 0, userThreads, tpool);
  // CachePagePoolTask::Init(tpool);

  arrResult = new Byte[rowNum];
  memset(arrResult, 0, rowNum);
  CreateDbTable();
  MVector<uint32_t> vctSessId;
  MVector<StmtResult> vctStmtRes(sessionNum);

  for (int i = 0; i < sessionNum; i++) {
    uint32_t sid = SessionPool::CreateSession(0, &vctStmtRes[i]);
    vctSessId.push_back(sid);
  }

  for (int i = 0; i < sessionNum; i++) {
    while (vctStmtRes[i].GetResultStatus() != ResultStatus::FINISHED) {
      this_thread::yield();
    }
  }

  MVector<thread *> vctThread;
  vctThread.reserve(userThreads);
  int sRange = sessionNum / userThreads;
  int rRange = rowNum / userThreads;
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
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << "Insert records Time(ms):" << duration.count()
           << "  Total Records: " << rowNum;

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

  TableManager::ClearTable();
  DatabaseManager::ClearDB();
  CachePagePool::ClearPool();
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  SessionPool::ClearPool();
  LogTask::Clear();

  LOG_INFO << "Memory leaked: " << CachePool::GetMemoryUsed();
#ifdef CACHE_TRACE
  unordered_map<uint64_t, string> &map = CachePool::_mapApply;
  for (auto iter = map.begin(); iter != map.end(); iter++) {
    LOG_INFO << (void *)iter->first << "    " << iter->second;
  }
#endif // CACHE_TRACE
}

} // namespace storage