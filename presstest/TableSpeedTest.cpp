#include "TableTestLib.h"

#include "../src/binlog/LogTask.h"
#include "../src/cache/Mallocator.h"
#include "../src/core/BranchPage.h"
#include "../src/core/IndexTree.h"
#include "../src/core/LeafPage.h"
#include "../src/core/LeafRecord.h"
#include "../src/expr/ExprStatement.h"
#include "../src/manager/DatabaseManager.h"
#include "../src/manager/TableManager.h"
#include "../src/pool/CachePagePool.h"
#include "../src/pool/FilePagePool.h"
#include "../src/serv/Session.h"
#include "../src/serv/SessionPool.h"
#include "../src/sql/Parser.h"
#include "../src/statement/StmtResult.h"
#include "../src/statement/TableSelectStatement.h"
#include "../src/table/Table.h"
#include "../src/table/TableTaskMgr.h"

namespace storage {
// ExprSelect *exprSel = nullptr;

// void SelectTestProc(int thd, uint32_t sessNum, Session *sess, int recStart,
//                     int recNum, int opTimes) {
//   ThreadPool::SetThreadId(thd);

//   MVector<StmtResult> vctResult(sessNum);
//   MVector<int> vctId(sessNum);
//   MVector<TableSelectStatement *> vctStmt(sessNum, nullptr);

//   int cnt = 0;
//   int times = 0;
//   srand(thd);
//   for (size_t i = 0; i < vctId.size(); i++) {
//     vctId[i] = -1;
//   }

//   while (true) {
//     bool empty = true;
//     times++;

//     for (size_t i = 0; i < sessNum; i++) {
//       TableSelectStatement *&stmt = vctStmt[i];
//       if (stmt == nullptr) {
//         if (cnt >= opTimes) {
//           continue;
//         }

//         // int currVal = cnt + rand() % 100 - 50;
//         // if (currVal >= recNum) {
//         //   currVal -= 50;
//         // } else if (currVal < 0) {
//         //   currVal += 50;
//         // }
//         // vctId[i] = recStart + currVal;

//         int currVal = recStart + cnt % recNum;
//         vctId[i] = currVal;
//         VectorDataValue vctDv = {new DataValueLong(GenPrimaryKey(currVal))};
//         ExprTableSelect *exprDest =
//             dynamic_cast<ExprTableSelect *>(exprSel->_exprDestSelect);
//         vctResult[i].Reset();
//         TranID txid = ((uint64_t)thd) << 40 + cnt;
//         stmt = new TableSelectStatement(cnt, txid, exprDest, move(vctDv),
//                                         &vctResult[i]);
//         cnt++;
//       }

//       StmtStatus status = stmt->SessionExec(sess);
//       if (status != StmtStatus::Finished) {
//         empty = false;
//         continue;
//       }

//       ResultStatus rs = vctResult[i].GetResultStatus();
//       assert(rs == ResultStatus::FINISHED);

//       StmtResult &rst = vctResult[i];
//       assert(!rst._bFailed && rst._rowNum == 1);
//       rst._resultSet->First();
//       VectorDataValue vctDv;
//       rst._resultSet->GetCurrDataValueRow(vctDv);
//       CheckSelectResult(vctId[i], vctDv);

//       delete stmt;
//       stmt = nullptr;
//     }

//     if (cnt >= opTimes) {
//       if (empty) {
//         break;
//       }
//     }
//   }
// }

void TestTableSpeed(int userThreads, int tblThreads, int rowNum,
                    int totalOpTimes) {
  //   ThreadPool *tpool =
  //       ThreadPool::CreateMainPool("press", 1, tblThreads + userThreads + 2);
  //   ThreadPool::SetThreadId(0);
  //   FilePagePool::Start(tblThreads + 3);
  //   LogTask::InitLogTask(tpool, "./binlog/", true);
  //   SessionPool::InitPool(userThreads, 1, 0, userThreads, tpool, true);
  //   CreateDbTable(true, userThreads);

  //   ParserResult result;
  //   bool b = Parser::Parse(SELECT_STMT, result);
  //   assert(b);

  //   MVectorPtr<ExprStatement *> *vctPtr = result.GetStatements();
  //   assert(vctPtr->size() == 1);

  //   exprSel = dynamic_cast<ExprSelect *>(vctPtr->at(0));
  //   vctPtr->clear();
  //   b = exprSel->Preprocess(DatabaseManager::FindDb(DB_NAME));
  //   assert(b);

  //   MVector<uint32_t> *vctSessId = new MVector<uint32_t>();
  //   vector<StmtResult> vctStmtRes(10);

  //   arrResult = new Byte[rowNum];
  //   memset(arrResult, 0, rowNum);

  //   for (int i = 0; i < 10; i++) {
  //     uint32_t sid = SessionPool::CreateSession(0, &vctStmtRes[i]);
  //     vctSessId->push_back(sid);
  //   }

  //   for (int i = 0; i < 10; i++) {
  //     while (vctStmtRes[i].GetResultStatus() != ResultStatus::FINISHED) {
  //       this_thread::yield();
  //     }
  //   }

  //   ThreadPool::PrintThreadTime();
  //   chrono::system_clock::time_point st = chrono::system_clock::now();

  //   thread *t = new thread(
  //       [vctSessId, rowNum]() { InsertProc(0, *vctSessId, 0, rowNum); });

  //   t->join();
  //   delete t;

  //   chrono::system_clock::time_point et = chrono::system_clock::now();
  //   ThreadPool::PrintThreadTime();
  //   auto duration =
  //       std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  //   LOG_INFO << "Insert records Time(ms):" << duration.count()
  //            << "  Total Records: " << rowNum;

  //   IndexPage *rootPage = table->GetVectorIndex()[0]._tree->GetRootPage();
  //   LOG_INFO << "Root RecordNumber: " << rootPage->GetRecordNumber()
  //            << "  PageLevel: " << (int)rootPage->GetPageLevel();

  //   TableTaskMgr::_dtLastWriteDisk = MicroSecTime();
  //   IndexRange &range = table->GetVectorIndex()[0]._tree->GetVctRange()[0];
  //   while (range._pageMap.size() > 1 || FilePagePool::IsBusy()) {
  //     this_thread::sleep_for(1us);
  //   }
  //   rootPage = table->GetVectorIndex()[0]._tree->GetRootPage();
  //   LOG_INFO << "Root RecordNumber: " << rootPage->GetRecordNumber()
  //            << "  PageLevel: " << (int)rootPage->GetPageLevel();

  //   IndexAdjustTask *adjustTask =
  //       new IndexAdjustTask(tpool, table->GetTableTaskMgr(), 0, tblThreads,
  //       true);
  //   tpool->AddTask(adjustTask);
  //   while (table->GetVectorIndex()[0]._tree->IsReranging()) {
  //     this_thread::yield();
  //   }

  //   ThreadPool::PrintThreadTime();
  //   vector<thread *> vctThread;
  //   vctThread.reserve(userThreads);

  //   st = chrono::system_clock::now();
  //   int opTimes = totalOpTimes / userThreads;
  //   int rRange = rowNum / userThreads;
  //   int sessNum = 10;
  //   Session *sess = SessionPool::GetSession(0);

  //   for (int i = 0; i < userThreads; i++) {
  //     int recStart = i * rRange;
  //     thread *t = new thread([i, sessNum, sess, recStart, rRange, opTimes]()
  //     {
  //       SelectTestProc(i, sessNum, sess, recStart, rRange, opTimes);
  //     });
  //     vctThread.push_back(t);
  //   }

  //   for (int i = 0; i < userThreads; i++) {
  //     vctThread[i]->join();
  //     delete vctThread[i];
  //   }

  //   vctThread.clear();
  //   et = chrono::system_clock::now();
  //   duration = std::chrono::duration_cast<std::chrono::milliseconds>(et -
  //   st); LOG_INFO << "Operator records Time(ms):" << duration.count()
  //            << "  Total times: " << totalOpTimes;
  //   ThreadPool::PrintThreadTime();
  //   CheckAllRecord(rowNum);
  //   PhysTable *tbl = nullptr;
  //   TableManager::FindTable(DB_TBL_NAME, tbl);
  //   tbl->GetTableTaskMgr()->SetMgrStatus(MgrStatus::SET_STOP);

  //   delete vctSessId;
  //   SessionPool::ClosePool();
  //   ThreadPool::CloseMainPool(true);
  //   FilePagePool::Stop();
  //   TableManager::ClearTable();
  //   DatabaseManager::ClearDB();
  //   SessionPool::ClearPool();
  //   CachePagePool::ClearPool();
  //   LogTask::Clear();
  //   _threadErrorMsg.reset();
  //   ErrorMsg::ClearErrorMsg();

  //   LOG_INFO << "Memory leaked: " << CachePool::GetMemoryUsed();
  // #ifdef CACHE_TRACE
  //   unordered_map<uint64_t, string> &map = CachePool::_mapApply;
  //   for (auto iter = map.begin(); iter != map.end(); iter++) {
  //     LOG_INFO << (void *)iter->first << "    " << iter->second;
  //   }
  // #endif // CACHE_TRACE
}
} // namespace storage