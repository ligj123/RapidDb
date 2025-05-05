
#include "../../src/binlog/LogTask.h"
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
BOOST_AUTO_TEST_CASE(Database_Statement_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const char *sql1 = "create database TEST_DB";
  const int SessNum = 1;

  ThreadPool *tpool = ThreadPool::CreateMainPool("test", 1, 8);
  tpool->SetStop();
  while (tpool->GetAliveThreadCount() != 0) {
    this_thread::yield();
  }

  uint16_t tidOld = ThreadPool::SetThreadId(0);
  LogTask::InitLogTask(tpool, "./binlog/");

  SessionPool::ClearPool();
  SessionPool::InitPool(1, 1, 0, 1, tpool);
  SessionTask *sessTask = SessionPool::GetVctSessionTask()[0];
  StmtResult result;
  uint32_t sessId = SessionPool::CreateSession(0, &result);
  TaskStatus s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  fs::path path = Configure::GetDbRootPath() + "rapid";
  if (fs::exists(path)) {
    fs::remove_all(path);
  }

  bool b = SysTable::InitSystemTable();
  assert(b);
  VectorRow vctRow;
  SessionPool::AddStatement(0, 0, 1, 1, sql1, move(vctRow), &result);
  sessTask->Run();

  PhysTable *dbTbl = nullptr;
  b = TableManager::FindTable(SysTable::SystemDbTableName(), dbTbl);
  assert(b);
  TableTaskMgr *mgr = dbTbl->GetTableTaskMgr();

  IndexTask *tblTask = mgr->GetVctIndexTasks()[0][0];
  s = tblTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = tblTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  LogTask::GetTask()->SetRemovedPool(true);
  CachePagePool::ClearPool();
  DatabaseManager::ClearDB();
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  SessionPool::ClearPool();
  LogTask::Clear();
  ThreadPool::SetThreadId(tidOld);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
