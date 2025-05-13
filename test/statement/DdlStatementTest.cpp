
#include "../../src/binlog/LogTask.h"
#include "../../src/core/LeafPage.h"
#include "../../src/manager/DatabaseManager.h"
#include "../../src/manager/TableManager.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/pool/FilePagePool.h"
#include "../../src/serv/Session.h"
#include "../../src/serv/SessionPool.h"
#include "../../src/statement/StmtResult.h"
#include "../../src/sysTable/SysTable.h"
#include "../../src/table/Database.h"
#include "../../src/table/Table.h"
#include "../../src/table/TableTaskMgr.h"

#include <boost/test/unit_test.hpp>
namespace fs = std::filesystem;

namespace storage {
BOOST_AUTO_TEST_SUITE(StatementTest)
BOOST_AUTO_TEST_CASE(DDL_Statement_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const char *sql1_1 = "create database test_db";
  const char *sql1_2 = "create database if not exists test_db";
  const char *sql1_3 = "drop database test_db";
  const char *sql1_4 = "drop database if exists test_db";
  const char *sql1_5 = "use test_db";
  const char *sql1_6 = "show databases";

  const char *sql2_1 =
      "create table test_db.test_tbl1(ii int primary key,jj int)";
  const char *sql2_2 = "create table test_tbl1(ii int primary key,jj int)";
  const char *sql2_3 =
      "create table if not exists test_tbl1(ii int primary key,jj int)";
  const char *sql2_4 = "show tables";
  const char *sql2_5 = "show tables from rapid";
  const char *sql2_6 = "drop table test_tbl1";
  const char *sql2_7 = "drop table if exists test_tbl1";

  const char *tran_sql1 = "begin";
  const char *tran_sql2 = "commit";
  const char *tran_sql3 = "rollback";
  const char *inst_sql = "insert into test_db.test_tbl1 values(?,?)";

  ThreadPool *tpool = ThreadPool::CreateMainPool("test", 1, 8);
  tpool->SetStop();
  while (tpool->GetAliveThreadCount() != 0) {
    this_thread::yield();
  }

  uint16_t tidOld = ThreadPool::SetThreadId(0);
  LogTask::InitLogTask(tpool, "./binlog/");
  FilePagePool::Start(1);

  SessionPool::ClearPool();
  SessionPool::InitPool(1, 1, 0, 1, tpool);
  SessionTask *sessTask = SessionPool::GetVctSessionTask()[0];
  StmtResult result;
  uint32_t sessId = SessionPool::CreateSession(0, &result);
  TaskStatus s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  fs::path path = Configure::GetDbRootPath();
  if (fs::exists(path)) {
    fs::remove_all(path);
  }

  // Create database
  bool b = SysTable::InitSystemTable();
  assert(b);
  VectorRow vctRow;
  SessionPool::AddStatement(0, 0, 1, 1, sql1_1, move(vctRow), &result);
  sessTask->Run();

  PhysTable *dbTbl = nullptr;
  b = TableManager::FindTable(SysTable::SystemDbTableName(), dbTbl);
  assert(b);
  TableTaskMgr *mgr = dbTbl->GetTableTaskMgr();

  IndexTask *dbTask = mgr->GetVctIndexTasks()[0][0];
  s = dbTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  Session *session = SessionPool::GetSession(sessId);
  session->_transaction.SetLogged();
  sessTask->Run();
#endif
  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(result._bFailed == false);

  Database *tdb = DatabaseManager::FindDb("test_db");
  BOOST_TEST(tdb != nullptr);
  BOOST_TEST(tdb->GetDbName() == "test_db");
  MString tpath = Configure::GetDbRootPath().c_str();
  tpath += "test_db_1";
  BOOST_TEST(tdb->GetDbPath() == tpath);

  LeafPage *lp = dbTbl->GetIndexTree(0)->GetBeginPage();
  VectorDataValue vctKey;
  vctKey.push_back(new DataValueInt(tdb->GetID()));
  RawKey key(vctKey);
  bool bFind = false;
  uint32_t pos = lp->SearchKey(key, bFind);
  BOOST_TEST(bFind);

  LeafRecord &lr = lp->GetRecord(pos);
  VectorDataValue vdv;
  ReleaseResult res = lp->ReleaseLock(&lr);
  BOOST_TEST(res == ReleaseResult::FINISHED);
  ReadResult rr = lr.ReadListValue({}, vdv, lp->GetIndexTree());
  BOOST_TEST(rr == ReadResult::OK_NOLOCK);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(vdv[1])) == "test_db");
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(vdv[2])) ==
             "test_db_1");

  // Creat an exist database
  SessionPool::AddStatement(0, 0, 2, 1, sql1_1, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._bFailed == true);
  BOOST_TEST(result._vctError[0] == "Database test_db has existed.");

  // Create an exist database with if not exist
  SessionPool::AddStatement(0, 0, 3, 3, sql1_2, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);
  BOOST_TEST(result._vctError.size() == 0);

  // Show database
  SessionPool::AddStatement(0, 0, 4, 4, sql1_6, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._resultSet->GetRowCount() == 2);
  BOOST_TEST(result._resultSet->First());
  IDataValue *dv = result._resultSet->GetDataValue(0);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(dv)) == "rapid");
  BOOST_TEST(result._resultSet->Next());
  dv = result._resultSet->GetDataValue(0);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(dv)) == "test_db");

  // Use database
  BOOST_TEST(session->_currDb == nullptr);
  SessionPool::AddStatement(0, 0, 5, 5, sql1_5, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(session->_currDb == tdb);

  // Create table
  PhysTable *sys_tbl = nullptr;
  b = TableManager::FindTable(SysTable::SystemTblTableName(), sys_tbl);
  IndexTask *sysTask = sys_tbl->GetTableTaskMgr()->GetVctIndexTasks()[0][0];

  SessionPool::AddStatement(0, 0, 6, 6, sql2_1, move(vctRow), &result);
  sessTask->Run();
  s = sysTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  session->_transaction.SetLogged();
  sessTask->Run();
#endif
  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(result._bFailed == false);

  PhysTable *test_tbl = nullptr;
  b = TableManager::FindTable("test_db.test_tbl1", test_tbl);
  BOOST_TEST(b);
  BOOST_TEST(test_tbl->GetDbName() == "test_db");
  BOOST_TEST(test_tbl->GetTableName() == "test_tbl1");

  lp = sys_tbl->GetIndexTree(0)->GetBeginPage();
  vctKey.clear();
  vctKey.push_back(new DataValueInt(test_tbl->TableID()));
  RawKey key2(vctKey);
  bFind = false;
  pos = lp->SearchKey(key2, bFind);
  BOOST_TEST(bFind);

  LeafRecord &lr2 = lp->GetRecord(pos);
  vdv.clear();
  res = lp->ReleaseLock(&lr2);
  BOOST_TEST(res == ReleaseResult::FINISHED);
  rr = lr2.ReadListValue({}, vdv, lp->GetIndexTree());
  BOOST_TEST(rr == ReadResult::OK_NOLOCK);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(vdv[1])) == "test_db");
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(vdv[2])) ==
             "test_tbl1");
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(vdv[3])) ==
             "test_tbl1_1");

  // Create exist table
  SessionPool::AddStatement(0, 0, 7, 7, sql2_2, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == true);
  BOOST_TEST(result._vctError[0] == "Table test_db.test_tbl1 has existed.");

  // Create table if exist
  SessionPool::AddStatement(0, 0, 8, 8, sql2_3, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);
  BOOST_TEST(result._vctError.size() == 0);

  // Show tables
  SessionPool::AddStatement(0, 0, 9, 9, sql2_4, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._resultSet->GetRowCount() == 1);
  BOOST_TEST(result._resultSet->First());
  dv = result._resultSet->GetDataValue(0);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(dv)) == "test_tbl1");

  // Show tables from
  SessionPool::AddStatement(0, 0, 10, 10, sql2_5, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._resultSet->GetRowCount() == 3);
  BOOST_TEST(result._resultSet->First());
  dv = result._resultSet->GetDataValue(0);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(dv)) == "sys_dbs");
  BOOST_TEST(result._resultSet->Next());
  dv = result._resultSet->GetDataValue(0);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(dv)) == "sys_tables");
  BOOST_TEST(result._resultSet->Next());
  dv = result._resultSet->GetDataValue(0);
  BOOST_TEST((MString)(*dynamic_cast<DataValueVarChar *>(dv)) == "sys_vars");

  // Drop table
  SessionPool::AddStatement(0, 0, 11, 11, sql2_6, move(vctRow), &result);
  sessTask->Run();
  s = sysTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  session->_transaction.SetLogged();
  sessTask->Run();
#endif
  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(result._bFailed == false);

  test_tbl = nullptr;
  b = TableManager::FindTable("test_db.test_tbl1", test_tbl);
  BOOST_TEST(!b);
  BOOST_TEST(lp->GetRecord(0).GetLock()->_actType == ActionType::DELETE);

  // Drop table if exists
  SessionPool::AddStatement(0, 0, 12, 12, sql2_7, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);
  BOOST_TEST(result._vctError.size() == 0);

  // Recreate table for drop database
  sys_tbl = nullptr;
  b = TableManager::FindTable(SysTable::SystemTblTableName(), sys_tbl);
  sysTask = sys_tbl->GetTableTaskMgr()->GetVctIndexTasks()[0][0];

  SessionPool::AddStatement(0, 0, 13, 13, sql2_1, move(vctRow), &result);
  sessTask->Run();
  s = sysTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  session->_transaction.SetLogged();
  sessTask->Run();
#endif
  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(result._bFailed == false);

  // Start Transaction
  SessionPool::AddStatement(0, 0, 30, 30, tran_sql1, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);

  // Insert a record
  b = TableManager::FindTable("test_db.test_tbl1", test_tbl);
  BOOST_TEST(b);
  IndexTask *testTask = test_tbl->GetTableTaskMgr()->GetVctIndexTasks()[0][0];
  vctRow.push_back({new DataValueInt(1), new DataValueInt(1)});
  SessionPool::AddStatement(0, 0, 31, 31, inst_sql, move(vctRow), &result);
  sessTask->Run();
  testTask->Run();
  sessTask->Run();

  // Commit Transaction
  SessionPool::AddStatement(0, 0, 32, 32, tran_sql2, move(vctRow), &result);
  sessTask->Run();
#ifndef WITHOUT_BIN_LOG
  session->_transaction.SetLogged();
  sessTask->Run();
#endif
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);

  lp = test_tbl->GetVectorIndex()[0]._tree->GetBeginPage();
  LeafRecord *plr = &lp->GetRecord(0);
  res = lp->ReleaseLock(plr);
  BOOST_TEST(res == ReleaseResult::FINISHED);
  vdv.clear();
  rr = plr->ReadListValue({}, vdv, lp->GetIndexTree());
  BOOST_TEST((int)(*dynamic_cast<DataValueInt *>(vdv[0])) == 1);
  BOOST_TEST((int)(*dynamic_cast<DataValueInt *>(vdv[1])) == 1);

  // Start Transaction
  SessionPool::AddStatement(0, 0, 33, 33, tran_sql1, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);

  // Insert the second record
  vctRow.push_back({new DataValueInt(2), new DataValueInt(2)});
  SessionPool::AddStatement(0, 0, 34, 34, inst_sql, move(vctRow), &result);
  sessTask->Run();
  testTask->Run();
  sessTask->Run();

  // Rollback Transaction
  SessionPool::AddStatement(0, 0, 35, 35, tran_sql3, move(vctRow), &result);
  sessTask->Run();

  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);
  plr = &lp->GetRecord(1);
  res = lp->ReleaseLock(plr);
  BOOST_TEST(res == ReleaseResult::DELETED);
  BOOST_TEST(plr->IsDelete());

  // Drop database
  SessionPool::AddStatement(0, 0, 14, 14, sql1_3, move(vctRow), &result);
  sessTask->Run();
  s = sysTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = dbTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  session->_transaction.SetLogged();
  sessTask->Run();
#endif
  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(result._bFailed == false);

  tdb = DatabaseManager::FindDb("test_db");
  BOOST_TEST(tdb == nullptr);
  lp = dbTbl->GetIndexTree(0)->GetBeginPage();
  BOOST_TEST(lp->GetRecord(0).GetLock()->_actType == ActionType::DELETE);

  lp->ReleaseLock(&lp->GetRecord(0));
  BOOST_TEST(lp->GetCommitedDataLength() == 0);
  BOOST_TEST(lp->GetTotalDataLength() == 0);

  // Drop unexist database with if exists
  SessionPool::AddStatement(0, 0, 15, 15, sql1_4, move(vctRow), &result);
  sessTask->Run();
  BOOST_TEST(result._status == ResultStatus::FINISHED);
  BOOST_TEST(result._bFailed == false);
  BOOST_TEST(result._vctError.size() == 0);

  TableManager::CloseTasksAndPages();
  this_thread::sleep_for(1ms);
  TableManager::ClearTable();
  DatabaseManager::ClearDB();

  vector<SessionTask *> &vctSessTask = SessionPool::GetVctSessionTask();
  for (SessionTask *task : vctSessTask) {
    task->SetStatus(TaskStatus::FINISHED, false);
    task->SetRemovedPool(true);
  }

  LogTask::GetTask()->SetRemovedPool(true);
  CachePagePool::ClearPool();
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  SessionPool::ClearPool();
  LogTask::Clear();
  ThreadPool::SetThreadId(tidOld);
}

BOOST_AUTO_TEST_CASE(Transaction_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
