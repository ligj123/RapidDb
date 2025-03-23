#include "PressTest.h"
#include "TableTestLib.h"

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
const int MAX_USER_THREADS = 8;
Byte *arrResult = nullptr;
int *arrNum = nullptr;
// The redio of insert, update , delete, select

ResultStat arrResStat[MAX_USER_THREADS];

void PrintNum(int rowNum, int num) {
  for (int i = 0; i < rowNum; i++) {
    if (arrNum[i] == num) {
      LOG_INFO << i;
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
  int times = 0;
  int endRec = startRec + recNum;

  while (true) {
    int empty = 0;
    times++;
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
      // StmtResult &rst = vctResult[i];

      // switch (pr.second) {
      // case OpRedio::INS:
      //   if (!rst._bFailed) {
      //     assert(arrResult[pr.first] == 0);
      //     arrResStat[tid]._insertPassed++;
      //     arrResult[pr.first] = 0x80;
      //   } else {
      //     assert(arrResult[pr.first] != 0);
      //     arrResStat[tid]._insertFailed++;
      //   }
      //   break;
      // case OpRedio::UPD:
      //   assert(!rst._bFailed);
      //   if (rst._rowNum > 0) {
      //     assert(arrResult[pr.first] >= 0x80);
      //     arrResStat[tid]._updatePassed++;
      //   } else {
      //     assert(arrResult[pr.first] == 0);
      //     arrResStat[tid]._updateFailed++;
      //   }
      //   break;
      // case OpRedio::DEL:
      //   assert(!rst._bFailed);
      //   if (rst._rowNum > 0) {
      //     assert(arrResult[pr.first] > 0);
      //     arrResStat[tid]._deletePassed++;
      //     arrResult[pr.first] = 0;
      //   } else {
      //     assert(arrResult[pr.first] == 0);
      //     arrResStat[tid]._deleteFailed++;
      //   }
      //   break;
      // case OpRedio::SEL:
      //   assert(!rst._bFailed);
      //   if (rst._rowNum > 0) {
      //     assert(arrResult[pr.first] > 0);
      //     arrResStat[tid]._selectPassed++;
      //     rst._resultSet->First();

      //     VectorDataValue vctDv;
      //     rst._resultSet->GetCurrDataValueRow(vctDv);
      //     CheckSelectResult(pr.first, vctDv);
      //   } else {
      //     assert(arrResult[pr.first] == 0);
      //     arrResStat[tid]._selectFailed++;
      //   }
      //   break;
      // default:
      //   abort();
      // }

      pr.first = -1;
    }

    if (cnt >= opTimes) {
      if (empty == vctSessId.size()) {
        break;
      }

      continue;
    }

    for (size_t i = 0; i < vctSessId.size() && cnt < opTimes; i++) {
      pair<int, OpRedio> &pr = vctPair[i];
      if (pr.first >= 0) {
        continue;
      }

      int currVal = cnt % recNum + rand() % 100 - 60 + startRec;
      if (currVal < startRec) {
        currVal = startRec;
      }

      while (true) {
        if (currVal >= endRec) {
          currVal = startRec;
        }

        if ((arrResult[currVal] & 0x40) == 0) {
          break;
        }

        currVal++;
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

  LOG_INFO << "Times: " << times;
}

void TablePointTest(uint16_t userThreads, uint16_t poolThreads,
                    uint16_t tblThreads, uint16_t sessGroupNum, int sessionNum,
                    int rowNum, int totalOpTimes, bool bExclusive) {
  assert(sessionNum % userThreads == 0);
  ThreadPool *tpool = ThreadPool::CreateMainPool("press", 1, poolThreads);
  ThreadPool::SetThreadId(0);
  FilePagePool::Start(poolThreads);
  // LogTask::InitLogTask(tpool, "./binlog/", bExclusive);
  SessionPool::InitPool(sessGroupNum, sessGroupNum, 0, userThreads, tpool,
                        bExclusive);
  // CachePagePoolTask::Init(tpool);

  arrResult = new Byte[rowNum];
  memset(arrResult, 0, rowNum);
  arrNum = new int[totalOpTimes];
  memset(arrNum, 0, totalOpTimes * 4);

  CreateDbTable(DB_NAME, bExclusive, sessGroupNum);
  vector<uint32_t> vctSessId;
  vector<StmtResult> vctStmtRes(sessionNum);
  Database *db = DatabaseManager::FindDb(DB_NAME);

  for (int i = 0; i < sessionNum; i++) {
    uint32_t sid = SessionPool::CreateSession(0, &vctStmtRes[i]);
    vctSessId.push_back(sid);
  }

  for (int i = 0; i < sessionNum; i++) {
    while (vctStmtRes[i].GetResultStatus() != ResultStatus::FINISHED) {
      this_thread::yield();
    }

    SessionUseDB *action =
        new SessionUseDB(vctSessId[i], &vctStmtRes[i], DB_NAME);
    SessionPool::AddAction(0, vctSessId[i], action, &vctStmtRes[i]);
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

  // for (int i = 0; i < userThreads; i++) {
  //   MVector<uint32_t> vct;
  //   vct.insert(vct.end(), vctSessId.begin() + i * sRange,
  //              vctSessId.begin() + (i + 1) * sRange);
  //   int recStart = i * rRange;
  //   thread *t = new thread(
  //       [i, vct, recStart, rRange]() { InsertProc(i, vct, recStart, rRange);
  //       });
  //   vctThread.push_back(t);
  // }

  // for (int i = 0; i < userThreads; i++) {
  //   vctThread[i]->join();
  //   delete vctThread[i];
  // }

  // vctThread.clear();
  chrono::system_clock::time_point et = chrono::system_clock::now();
  // ThreadPool::PrintThreadTime();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  // LOG_INFO << "Insert records Time(ms):" << duration.count()
  //          << "  Total Records: " << rowNum;

  PhysTable *table;
  bool b = TableManager::FindTable(DB_TBL_NAME, table);
  assert(b);

  // IndexPage *rootPage = table->GetVectorIndex()[0]._tree->GetRootPage();
  // LOG_INFO << "Root RecordNumber: " << rootPage->GetRecordNumber()
  //          << "  PageLevel: " << (int)rootPage->GetPageLevel();

  // TableTaskMgr::_dtLastWriteDisk = MicroSecTime();
  // IndexRange &range = table->GetVectorIndex()[0]._tree->GetVctRange()[0];
  // while (range._pageMap.size() > 1 || FilePagePool::IsBusy()) {
  //   this_thread::sleep_for(1us);
  // }
  // rootPage = table->GetVectorIndex()[0]._tree->GetRootPage();
  // LOG_INFO << "Root RecordNumber: " << rootPage->GetRecordNumber()
  //          << "  PageLevel: " << (int)rootPage->GetPageLevel();

  // if (tblThreads > 1) {
  //   IndexAdjustTask *adjustTask =
  //       new IndexAdjustTask(tpool, table->GetTableTaskMgr(), 0, 2, true);
  //   tpool->AddTask(adjustTask);
  //   while (table->GetTableTaskMgr()->GetVctIndexTasks()[0].size() != 2) {
  //     this_thread::yield();
  //   }
  // }

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

  MVector<IndexTask *> vTask = table->GetTableTaskMgr()->GetVctIndexTasks()[0];
  stringstream ss;
  ss << "Action Number:  ";
  for (size_t i = 0; i < vTask.size(); i++) {
    ss << i << "  " << vTask[i]->GetActionCount() << "\t";
  }

  LOG_INFO << ss.str();

  ThreadPool::PrintThreadTime();
  CheckAllRecord(DB_TBL_NAME, rowNum);
  table->GetTableTaskMgr()->SetMgrStatus(MgrStatus::SET_STOP);
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
  // LogTask::Clear();
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