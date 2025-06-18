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
#include "../src/sql/Parser.h"
#include "../src/statement/StmtResult.h"
#include "../src/table/Table.h"
#include "../src/table/TableTaskMgr.h"

namespace storage {

void OperateProc1(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                  int recNum, int opTimes, int multi) {
  int cnt = 0;
  int times = 0;
  MVector<StmtResult> vctResult(vctSessId.size());
  MVector<int> vctVal(vctSessId.size());
  // int32_t poolSz = SessionPool::GetVctSessionGroup().size();

  while (true) {
    int empty = 0;
    times++;
    // MTreeMap<uint32_t, MVector<SessionStatementAction *>> mapAct;

    for (size_t i = 0; i < vctSessId.size(); i++) {
      ResultStatus rs = vctResult[i].GetResultStatus();
      if (rs == ResultStatus::FILLING) {
        continue;
      }

      if (cnt >= opTimes) {
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

      int currVal = cnt % recNum + MicroSecTime() % 100 - 50;
      if (currVal >= recNum) {
        currVal -= 50;
      } else if (currVal < 0) {
        currVal += 50;
      }

      currVal += startRec;
      vctVal[i] = currVal;
      VectorRow vctRow;
      vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
      SessionPool::AddStatement(tid, vctSessId[i], cnt + startRec, 4,
                                SELECT_STMT, move(vctRow), &vctResult[i]);

      // SessionStatementAction *action =
      //     new SessionStatementAction(vctSessId[i], cnt + startRec, 4,
      //                                SELECT_STMT, move(vctRow),
      //                                &vctResult[i]);
      // uint32_t key = ((vctSessId[i] % poolSz) << 16) + tid;
      // auto iter = mapAct.try_emplace(key, MVector<SessionStatementAction
      // *>()); iter.first->second.push_back(action);

      cnt++;
    }

    // SessionPool::AddStatements(mapAct);

    if (empty >= vctSessId.size()) {
      break;
    }
  }

  LOG_INFO << "Times: " << times;
}

void OperateProc2(uint16_t tid, MVector<uint32_t> vctSessId, int startRec,
                  int recNum, int opTimes, int multi) {
  // LOG_INFO << "tid: " << tid << "  SessNum: " << vctSessId.size()
  //          << "   startRec: " << startRec << "   recNum: " << recNum
  //          << "  opTimes: " << opTimes;
  int cnt = -1;
  int times = 0;
  MVector<StmtResultEx> vctResult(vctSessId.size() * multi);
  int currSess = -1;
  int waitRst = recNum > vctResult.size() ? vctResult.size() : recNum;
  int32_t poolSz = SessionPool::GetVctSessionGroup().size();
  MVector<StmtResultEx *> vctRes;
  vctRes.reserve(vctResult.size());

  while (true) {
    times++;
    vctRes.clear();

    for (StmtResultEx &res : vctResult) {
      ResultStatus s = res.GetResultRelax();
      if (s == ResultStatus::FILLING) {
        continue;
      }

      if (s == ResultStatus::FINISHED && res._currVal >= 0) {
        VectorDataValue vctDv;

        bool b = res._resultSet->First();
        assert(b);
        res._resultSet->GetCurrDataValueRow(vctDv);
        CheckSelectResult(res._currVal, vctDv);
      }

      vctRes.push_back(&res);
    }

    if (vctRes.size() == 0) {
      this_thread::yield();
      continue;
    }

    if (vctRes.size() == vctResult.size() && cnt >= opTimes) {
      break;
    }

    vctRes[0]->GetResultStatus();
    //  MTreeMap<uint32_t, MVector<SessionStatementAction *>> mapAct;

    for (StmtResultEx *res : vctRes) {
      cnt++;
      currSess++;
      if (currSess >= vctSessId.size()) {
        currSess = 0;
      }

      int currVal = cnt % recNum + MicroSecTime() % 100 - 50;
      if (currVal >= recNum) {
        currVal -= 50;
      } else if (currVal < 0) {
        currVal += 50;
      }

      currVal += startRec;
      res->_currVal = currVal;
      VectorRow vctRow;
      vctRow.push_back({new DataValueLong(GenPrimaryKey(currVal))});
      SessionPool::AddStatement(tid, vctSessId[currSess], currVal, 4,
                                SELECT_STMT, move(vctRow), res);

      // SessionStatementAction *action = new SessionStatementAction(
      //     vctSessId[currSess], currVal, 4, SELECT_STMT, move(vctRow), res);
      // uint32_t key = ((vctSessId[currSess] % poolSz) << 16) + tid;
      // auto iter = mapAct.try_emplace(key, MVector<SessionStatementAction
      // *>()); iter.first->second.push_back(action);
    }

    // SessionPool::AddStatements(mapAct);
  }

  LOG_INFO << "Times: " << times;
}

void TestMultiTable(int tblNum, int sessGroupNum, int sessNum, int rowNum,
                    int totalOpTimes, int multi) {
  ThreadPool *tpool =
      ThreadPool::CreateMainPool("press", 1, tblNum * 2 + sessGroupNum + 2);
  FilePagePool::Start(tblNum * 2 + sessGroupNum + 2);
  ThreadPool::SetThreadId(0);
#ifdef WITHOUT_BIN_LOG
  LogTask::InitLogTask(tpool, "./binlog/", true);
#endif
  SessionPool::InitPool(sessGroupNum, sessGroupNum, 0, tblNum, tpool, true);
  vector<MVector<uint32_t>> vctArrSessId;
  vector<StmtResult> vctStmtRes(sessNum);
  arrResult = new Byte[rowNum];
  memset(arrResult, 0, rowNum);
  vector<SessionGroup> &vctGroup = SessionPool::GetVctSessionGroup();
  for (int i = 0; i < tblNum; i++) {
    MString dbName = DB_NAME + ToMString(i);
    CreateDbTable(dbName.c_str(), true, sessGroupNum);

    for (SessionGroup &group : vctGroup) {
      ParserResult result;
      bool b = Parser::Parse(INSERT_STMT, result);
      assert(b);
      MVectorPtr<ExprStatement *> *vctPtr = result.GetStatements();
      ExprStatement *exprStmt = vctPtr->at(0);
      vctPtr->clear();
      b = exprStmt->Preprocess(DatabaseManager::FindDb(dbName));
      assert(b);
      group._mapSqlExprStatement.emplace(dbName + INSERT_STMT, exprStmt);
    }

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
  chrono::system_clock::time_point st = chrono::system_clock::now();

  for (int i = 0; i < tblNum; i++) {
    int recStart = i * rRange;
    thread *t = new thread([i, vctArrSessId, recStart, rRange, multi]() {
      InsertProc1(i, vctArrSessId[i], recStart, rRange, multi);
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

  ThreadPool::PrintThreadTime();
  TableTaskMgr::_dtLastWriteDisk = MicroSecTime();
  this_thread::sleep_for(3s);

  st = chrono::system_clock::now();
  int opTimes = totalOpTimes / tblNum;
  for (int i = 0; i < tblNum; i++) {
    int recStart = i * rRange;
    thread *t =
        new thread([i, vctArrSessId, recStart, rRange, opTimes, multi]() {
          OperateProc1(i, vctArrSessId[i], recStart, rRange, opTimes, multi);
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
  ThreadPool::PrintThreadTime();
  for (int i = 0; i < tblNum; i++) {
    PhysTable *tbl = nullptr;
    string name = FulleTblName(DB_NAME, i, TBL_NAME);
    TableManager::FindTable(name.c_str(), tbl);
    tbl->GetTableTaskMgr()->SetMgrStatus(MgrStatus::SET_STOP);
  }

  this_thread::sleep_for(1s);
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