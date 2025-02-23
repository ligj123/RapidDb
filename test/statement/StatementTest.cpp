
#include "../../src/binlog/LogTask.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/manager/DatabaseManager.h"
#include "../../src/manager/TableManager.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/pool/FilePagePool.h"
#include "../../src/serv/SessionPool.h"
#include "../../src/sql/Parser.h"
#include "../../src/statement/DeleteStatement.h"
#include "../../src/statement/InsertStatement.h"
#include "../../src/statement/TableSelectStatement.h"
#include "../../src/statement/UpdateStatement.h"
#include "../../src/table/TableTaskMgr.h"
#include "../../src/utils/Log.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>

namespace storage {
const MString TABLE_NAME = "t1";
const MString DB_NAME = "testDb";
static uint32_t stmtId{0};
static TranID txID{0};

MVector<int> GenerateInt(int send, int num, MTreeSet<int> &mset) {
  std::srand(send);
  MVector<int> mvct;
  mvct.reserve(num);
  while (mvct.size() < num) {
    int val = std::rand();
    if (mset.insert(val).second) {
      mvct.push_back(val);
    }
  }

  return mvct;
}

VectorRow GenRecords(const MVector<int> &mvct) {
  MString str = "VARCHAR_50_" + MString(20, 'a');
  VectorRow vctRow;
  vctRow.reserve(mvct.size());

  for (int ival : mvct) {
    VectorDataValue vctDv;
    DataValueLong *dvLong = new DataValueLong((int64_t)ival * ival + ival);
    DataValueInt *dvInt = new DataValueInt(BytesSwap32(ival));

    stringstream ss;
    ss << "_0x" << std::setfill('0') << std::setw(8) << std::hex << ival;
    MString val = str + ss.str().c_str();
    DataValueVarChar *dvVar = new DataValueVarChar(val.c_str(), val.size(), 50);

    vctDv.push_back(dvLong);
    vctDv.push_back(dvInt);
    vctDv.push_back(dvVar);
    vctRow.push_back(move(vctDv));
  }

  return vctRow;
}

PhysTable *CreateTable(Database *db, const MString &tableName) {
  PhysTable *ptable =
      new PhysTable(db, tableName, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::LONG, false, -1, "primary key",
                    Charsets::UNKNOWN, nullptr);
  ptable->AddColumn("c2", DataType::INT, false, -1, "Unique Key",
                    Charsets::UNKNOWN, nullptr);
  ptable->AddColumn("c3", DataType::VARCHAR, true, 50, "NonUnique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddIndex(IndexType::PRIMARY, PRIMARY_KEY, {"c1"});
  ptable->AddIndex(IndexType::UNIQUE, "c2_unique", {"c2"});
  ptable->AddIndex(IndexType::NON_UNIQUE, "c3_non_unique", {"c3"});

  bool b = ptable->OpenIndex(0, true);
  BOOST_TEST(b);
  b = ptable->OpenIndex(1, true);
  BOOST_TEST(b);
  b = ptable->OpenIndex(2, true);
  BOOST_TEST(b);

  return ptable;
}

ExprStatement *CreateExprStatement(Database *db, const MString &sql) {
  ParserResult result;
  bool b = Parser::Parse(sql, result);
  BOOST_TEST(b);
  BOOST_TEST(result.IsValid());

  ExprStatement *exprStmt = result.RemoveFirstStatement();
  b = exprStmt->Preprocess(db);
  BOOST_TEST(b);
  return exprStmt;
}

BOOST_AUTO_TEST_SUITE(StatementTest)
BOOST_AUTO_TEST_CASE(Statement_Task_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  uint16_t tidOld = ThreadPool::SetThreadId(0);
  FilePagePool::Start(8);

  Database *db = new Database(1, ROOT_PATH.c_str(), DB_NAME, MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *table = CreateTable(db, TABLE_NAME);
  TableManager::AddTable(DB_NAME + "." + TABLE_NAME, table);

  ThreadPool *tpool = ThreadPool::CreateMainPool("test", 1, 8);
  tpool->SetStop();
  while (tpool->GetAliveThreadCount() != 0) {
    this_thread::yield();
  }
  LogTask::InitLogTask(tpool, "./binlog/");

  SessionPool::ClearPool();
  SessionPool::InitPool(1, 1, 0, 1, tpool);
  TableTaskMgr *tmgr = new TableTaskMgr(tpool, table, 1);
  table->SetTableTaskMgr(tmgr);
  MVector<IndexProp> &vctProp = table->GetVectorIndex();

  MTreeSet<int> setInt;
  MVector<int> vctInt = GenerateInt(123, 100, setInt);
  VectorRow vctRow = GenRecords(vctInt);

  uint32_t sid = 0;
  SessionGroup &sGroup = SessionPool::GetVctSessionGroup()[0];
  Session *session = new Session(sid);
  sGroup._mapSession.emplace(sid, session);
  session->_currDb = db;

  // Insert records
  ExprInsert *exprInst = dynamic_cast<ExprInsert *>(
      CreateExprStatement(db, "insert into t1 values(?,?,?)"));
  StmtResult stmtResult;
  InsertStatement *stmt = new InsertStatement(stmtId++, TXID_NULL, exprInst,
                                              move(vctRow), &stmtResult);
  session->_lstWaittingStmt.push_back(stmt);
  session->Exec();

  MVector<IndexTaskQueue *> &vctTaskQueue = tmgr->GetIndexTaskQueue();
  BOOST_TEST(vctTaskQueue.size() == 3);
  BOOST_TEST(vctTaskQueue[0]->_queueSessionAction.RoughSize() == 100);

  MVector<MVector<IndexTask *>> &vctTasks = tmgr->GetVctIndexTasks();
  BOOST_TEST(vctTasks.size() == 3);

  TaskStatus s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  SessionTask *sessTask = SessionPool::GetVctSessionTask()[0];
  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  MList<SessionAction *> lst;
  sGroup._threaPoolQueue.Pop(lst);
  assert(lst.size() == 0);
  BOOST_TEST(stmt->GetStmtStatus() == StmtStatus::Logging);

  LogTask *logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  BOOST_TEST(session->_currStatement == nullptr);
  delete exprInst;

  // Select records with primary key query
  MString msql = "select * from t1 where c1=?";
  VectorRow vctParas;
  int64_t val = (int64_t)vctInt[5] * vctInt[5] + vctInt[5];
  vctParas.push_back({new DataValueLong(val)});

  SessionPool::AddStatement(0, 0, stmtId++, 1, move(msql), move(vctParas),
                            &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  sGroup._threaPoolQueue.Pop(lst);
  assert(lst.size() == 0);

  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  BOOST_TEST(session->_currStatement == nullptr);

  // Clear
  CachePagePool::ClearPool();
  DatabaseManager::ClearDB();
  ThreadPool::SetThreadId(tidOld);
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  SessionPool::ClearPool();
  LogTask::Clear();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
