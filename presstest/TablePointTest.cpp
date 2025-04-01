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

void StatementProc1(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                    int recNum, int opTimes, int multi) {
  MVector<StmtResultEx> vctResult(vctSessId.size());

  int cnt = 0;
  int times = 0;
  int endRec = startRec + recNum;
  // int32_t poolSz = SessionPool::GetVctSessionGroup().size();

  while (true) {
    bool empty = true;
    times++;
    // MTreeMap<uint32_t, MVector<SessionStatementAction *>> mapAct;

    for (size_t i = 0; i < vctSessId.size(); i++) {
      StmtResultEx &rst = vctResult[i];
      ResultStatus rs = rst.GetResultStatus();
      if (rs == ResultStatus::FILLING) {
        empty = false;
        continue;
      }

      if (cnt >= opTimes) {
        continue;
      }

      int currVal = cnt % recNum + MicroSecTime() % 100 - 50;
      if (currVal >= recNum) {
        currVal -= 50;
      } else if (currVal < 0) {
        currVal += 50;
      }
      currVal += startRec;

      arrNum[cnt] = currVal;
      rst._opRedio = arrRadio[cnt % redioCount];
      rst._currVal = currVal;

      VectorRow vctRow;
      vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
      SessionPool::AddStatement(tid, vctSessId[i], currVal, 4, SELECT_STMT,
                                move(vctRow), &rst);

      // SessionStatementAction *action =
      //     new SessionStatementAction(vctSessId[i], cnt + startRec, 4,
      //                                SELECT_STMT, move(vctRow), &rst);
      // uint32_t key = ((vctSessId[i] % poolSz) << 16) + tid;
      // auto iter = mapAct.try_emplace(key, MVector<SessionStatementAction
      // *>()); iter.first->second.push_back(action);

      cnt++;
    }

    // SessionPool::AddStatements(mapAct);

    if (cnt >= opTimes && empty) {
      break;
    }
  }

  LOG_INFO << "Times: " << times;
}

void StatementProc2(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                    int recNum, int opTimes, int multi) {
  MVector<StmtResultEx> vctResult(vctSessId.size() * multi);
  int currRst = -1;
  int waitRst = recNum > vctResult.size() ? vctResult.size() : recNum;

  int cnt = 0;
  int times = 0;
  int endRec = startRec + recNum;
  // int32_t poolSz = SessionPool::GetVctSessionGroup().size();

  while (true) {
    times++;
    //  MTreeMap<uint32_t, MVector<SessionStatementAction *>> mapAct;

    for (size_t i = 0; i < vctSessId.size(); i++) {
      while (true) {
        currRst++;
        if (currRst >= vctResult.size()) {
          currRst = 0;
        }

        StmtResultEx &rst = vctResult[currRst];
        ResultStatus rs = rst.GetResultStatus();
        if (rs == ResultStatus::FILLING) {
          continue;
        }

        if (rs == ResultStatus::FINISHED && rst._currVal >= 0) {
          // Check results
        }

        break;
      }

      if (cnt >= opTimes) {
        if (vctResult[currRst]._currVal >= 0) {
          waitRst--;
          vctResult[currRst]._currVal = -1;
        }

        if (waitRst == 0) {
          break;
        } else {
          continue;
        }
      }

      int currVal = cnt % recNum + MicroSecTime() % 100 - 50;
      if (currVal >= recNum) {
        currVal -= 50;
      } else if (currVal < 0) {
        currVal += 50;
      }

      currVal += startRec;
      vctResult[currRst]._currVal = currVal;
      vctResult[currRst]._opRedio = arrRadio[cnt % redioCount];

      VectorRow vctRow;
      vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
      SessionPool::AddStatement(tid, vctSessId[i], cnt + startRec, 4,
                                SELECT_STMT, move(vctRow), &vctResult[currRst]);

      // SessionStatementAction *action =
      //     new SessionStatementAction(vctSessId[i], cnt + startRec, 4,
      //                                SELECT_STMT, move(vctRow),
      //                                &vctResult[i]);
      // uint32_t key = ((vctSessId[i] % poolSz) << 16) + tid;
      // auto iter = mapAct.try_emplace(key, MVector<SessionStatementAction
      // *>()); iter.first->second.push_back(action);

      cnt++;
    }

    //  SessionPool::AddStatements(mapAct);

    if (waitRst == 0) {
      break;
    }
  }

  LOG_INFO << "Times: " << times << "   Count: " << cnt;
}

void TablePointTest(uint16_t userThreads, uint16_t tblThreads,
                    uint16_t sessGroupNum, int sessionNum, int rowNum,
                    int totalOpTimes, int multi) {
  sessionNum *= sessGroupNum;
  ThreadPool *tpool =
      ThreadPool::CreateMainPool("press", 1, tblThreads + sessGroupNum + 2);
  ThreadPool::SetThreadId(0);
  FilePagePool::Start(tblThreads + sessGroupNum + 2);
  // LogTask::InitLogTask(tpool, "./binlog/", true);
  SessionPool::InitPool(sessGroupNum, sessGroupNum, 0, userThreads, tpool,
                        true);
  // CachePagePoolTask::Init(tpool);

  arrResult = new Byte[rowNum];
  memset(arrResult, 0, rowNum);
  arrNum = new int[totalOpTimes];
  memset(arrNum, 0, totalOpTimes * 4);

  CreateDbTable(DB_NAME, true, sessGroupNum);
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

  {
    MVector<uint32_t> vct;
    vct.insert(vct.end(), vctSessId.begin(), vctSessId.end());
    vctThread[0] = new thread(
        [vct, rowNum, multi]() { InsertProc1(0, vct, 0, rowNum, multi); });
  }

  vctThread[0]->join();
  delete vctThread[0];

  vctThread.clear();
  chrono::system_clock::time_point et = chrono::system_clock::now();
  // ThreadPool::PrintThreadTime();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << "Insert records Time(ms):" << duration.count()
           << "  Total Records: " << rowNum;

  PhysTable *table;
  bool b = TableManager::FindTable(DB_TBL_NAME, table);
  assert(b);

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

  if (tblThreads > 1) {
    IndexAdjustTask *adjustTask = new IndexAdjustTask(
        tpool, table->GetTableTaskMgr(), 0, tblThreads, true);
    tpool->AddTask(adjustTask);
    this_thread::sleep_for(3s);
  } else {
    this_thread::sleep_for(3s);
  }

  ThreadPool::PrintThreadTime();
  st = chrono::system_clock::now();
  int opTimes = totalOpTimes / userThreads;
  for (int i = 0; i < userThreads; i++) {
    MVector<uint32_t> vct;
    vct.insert(vct.end(), vctSessId.begin() + i * sRange,
               vctSessId.begin() + (i + 1) * sRange);
    int recStart = i * rRange;
    thread *t = new thread([i, vct, recStart, rRange, opTimes, multi]() {
      StatementProc1(i, vct, recStart, rRange, opTimes, multi);
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

  MVector<IndexTask *> &vTask = table->GetTableTaskMgr()->GetVctIndexTasks()[0];
  stringstream ss;
  ss << "Action Number:  ";
  for (size_t i = 0; i < vTask.size(); i++) {
    ss << i << ".  " << vTask[i]->GetActionCount() << "\t";
  }

  LOG_INFO << ss.str();

  ThreadPool::PrintThreadTime();
  // CheckAllRecord(DB_TBL_NAME, rowNum);
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