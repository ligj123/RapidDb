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

void OperateProc(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                 int recNum, int opTimes) {
  srand(tid);
  int cnt = 0;
  int times = 0;
  MVector<StmtResult> vctResult(vctSessId.size());
  MVector<int> vctVal(vctSessId.size());

  while (true) {
    int empty = 0;
    times++;
    for (size_t i = 0; i < vctSessId.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs == ResultStatus::FILLING) {
        continue;
      }

      if (cnt >= recNum) {
        empty++;
        continue;
      }

      if (rs == ResultStatus::FINISHED) {
        VectorDataValue vctDv;
        StmtResult &rst = vctResult[i];
        bool b = rst._resultSet->First();
        assert(b);
        rst._resultSet->GetCurrDataValueRow(vctDv);

        CheckSelectResult(vctVal[i], vctDv);
      }

      int currVal = cnt % recNum + rand() % 1000 - 500;
      if (currVal >= recNum) {
        currVal -= 500;
      } else if (currVal < 0) {
        currVal += 500;
      }

      currVal += startRec;
      vctVal[i] = currVal;
      VectorRow vctRow;
      vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
      SessionPool::AddStatement(tid, vctSessId[i], cnt + startRec, 4,
                                SELECT_STMT, move(vctRow), &vctResult[i]);
      cnt++;
    }

    if (empty >= vctSessId.size()) {
      break;
    }
  }
}

void TestMultiTable(int tblNum, int sessGroupNum, int sessNum, int rowNum,
                    int totalOpTimes) {
  ThreadPool *tpool =
      ThreadPool::CreateMainPool("press", 1, tblNum * 2 + sessGroupNum + 2);
  FilePagePool::Start(tblNum * 2 + sessGroupNum + 2);
  ThreadPool::SetThreadId(0);
  LogTask::InitLogTask(tpool, "./binlog/", true);
  SessionPool::InitPool(sessGroupNum, sessGroupNum, 0, tblNum, tpool, true);
  vector<MVector<uint32_t>> vctArrSessId;
  vector<StmtResult> vctStmtRes(sessNum);
  arrResult = new Byte[rowNum];
  memset(arrResult, 0, rowNum);

  for (int i = 0; i < sessGroupNum; i++) {
    MString dbName = DB_NAME + ToMString(i);
    CreateDbTable(dbName.c_str(), true, sessGroupNum);

    MVector<uint32_t> vct;
    vct.reserve(sessNum);
    for (int j = 0; j < sessNum; j++) {
      uint32_t id = SessionPool::CreateSession(0, &vctStmtRes[j]);
      vct.push_back(id);
    }

    for (int j = 0; j < sessNum; j++) {
      while (vctStmtRes[j].GetResultStatus() != ResultStatus::FINISHED) {
        this_thread::yield();
      }

      SessionUseDB *action = new SessionUseDB(vct[j], &vctStmtRes[j], dbName);
      SessionPool::AddAction(0, vct[j], action, &vctStmtRes[j]);
    }

    for (int j = 0; j < sessNum; j++) {
      while (vctStmtRes[j].GetResultStatus() != ResultStatus::FINISHED) {
        this_thread::yield();
      }
    }
    vctArrSessId.push_back(std::move(vct));
  }

  vector<thread *> vctThread;
  vctThread.reserve(tblNum);
  int rRange = rowNum / tblNum;
  ThreadPool::PrintThreadTime();
  chrono::system_clock::time_point st = chrono::system_clock::now();

  for (int i = 0; i < tblNum; i++) {
    int recStart = i * rRange;
    thread *t = new thread([i, vctArrSessId, recStart, rowNum]() {
      InsertProc(i, vctArrSessId[i], recStart, rowNum);
    });
    vctThread.push_back(t);
  }

  for (int i = 0; i < tblNum; i++) {
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

  st = chrono::system_clock::now();
  int opTimes = totalOpTimes / tblNum;
  for (int i = 0; i < tblNum; i++) {
    int recStart = i * rRange;
    thread *t = new thread([i, vctArrSessId, recStart, rowNum, totalOpTimes]() {
      OperateProc(i, vctArrSessId[i], recStart, rowNum, totalOpTimes);
    });
    vctThread.push_back(t);
  }

  for (int i = 0; i < 1; i++) {
    vctThread[i]->join();
    delete vctThread[i];
  }

  vctThread.clear();
  et = chrono::system_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  LOG_INFO << "Operator records Time(ms):" << duration.count()
           << "  Total times: " << totalOpTimes;

  for (int i = 0; i < tblNum; i++) {
    PhysTable *tbl = nullptr;
    string name = FulleTblName(DB_NAME, i, TBL_NAME);
    TableManager::FindTable(name.c_str(), tbl);
    tbl->GetTableTaskMgr()->SetMgrStatus(MgrStatus::SET_STOP);
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
  vctArrSessId.clear();

  LOG_INFO << "Memory leaked: " << CachePool::GetMemoryUsed();
#ifdef CACHE_TRACE
  unordered_map<uint64_t, string> &map = CachePool::_mapApply;
  for (auto iter = map.begin(); iter != map.end(); iter++) {
    LOG_INFO << (void *)iter->first << "    " << iter->second;
  }
#endif // CACHE_TRACE
}
} // namespace storage