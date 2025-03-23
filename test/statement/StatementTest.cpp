
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
static uint32_t StmtId{0};
static TranID txID{0};
static uint32_t ExprId{1};

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

int GetPosInt(MTreeSet<int> &mset, int pos) {
  auto iter = mset.begin();
  for (int i = 0; i < pos; i++) {
    iter++;
  }

  return *iter;
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

MString GenMString(int ival) {
  stringstream ss;
  ss << "_0x" << std::setfill('0') << std::setw(8) << std::hex << ival;
  return "VARCHAR_50_" + MString(20, 'a') + ss.str().c_str();
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

void CheckDeleteRecord(PhysTable *table, int val, uint32_t recNum) {
  LeafPage *priPage =
      dynamic_cast<LeafPage *>(table->GetVectorIndex()[0]._tree->GetRootPage());
  LeafPage *untPage =
      dynamic_cast<LeafPage *>(table->GetVectorIndex()[1]._tree->GetRootPage());
  LeafPage *nonPage =
      dynamic_cast<LeafPage *>(table->GetVectorIndex()[2]._tree->GetRootPage());

  BOOST_TEST(priPage->GetRecordNumber() == recNum);
  RawKey priKey({new DataValueLong((int64_t)val * val + val)});
  bool bFind;
  int pos = priPage->SearchKey(priKey, bFind);
  BOOST_TEST(bFind);
  LeafRecord *lr = &priPage->GetRecord(pos);
  BOOST_TEST(lr->IsDelete());
  BOOST_TEST(lr->GetLock()->_actType == ActionType::DELETE);
  BOOST_TEST(lr->GetLock()->_recStatus == RecordStatus::COMMITED);

  BOOST_TEST(untPage->GetRecordNumber() == recNum);
  RawKey uniKey({new DataValueInt((int32_t)BytesSwap32(val))});
  pos = untPage->SearchKey(uniKey, bFind);
  BOOST_TEST(bFind);
  lr = &untPage->GetRecord(pos);
  BOOST_TEST(lr->IsDelete());
  BOOST_TEST(lr->GetLock()->_actType == ActionType::DELETE);
  BOOST_TEST(lr->GetLock()->_recStatus == RecordStatus::COMMITED);

  BOOST_TEST(nonPage->GetRecordNumber() == recNum);
  MString ss = GenMString(val);
  RawKey nonKey({new DataValueVarChar(ss.c_str(), ss.size(), 50)});
  pos = nonPage->SearchKey(nonKey, bFind);
  BOOST_TEST(bFind);
  lr = &nonPage->GetRecord(pos);
  BOOST_TEST(lr->IsDelete());
  BOOST_TEST(lr->GetLock()->_actType == ActionType::DELETE);
  BOOST_TEST(lr->GetLock()->_recStatus == RecordStatus::COMMITED);
}

void CheckUpdateRecord(PhysTable *table, int priVal, int unqVal, int nonVal) {
  LeafPage *priPage =
      dynamic_cast<LeafPage *>(table->GetVectorIndex()[0]._tree->GetRootPage());
  LeafPage *untPage =
      dynamic_cast<LeafPage *>(table->GetVectorIndex()[1]._tree->GetRootPage());
  LeafPage *nonPage =
      dynamic_cast<LeafPage *>(table->GetVectorIndex()[2]._tree->GetRootPage());

  int64_t pval = (int64_t)priVal * priVal + priVal;
  RawKey priKey({new DataValueLong(pval)});
  bool bFind;
  int pos = priPage->SearchKey(priKey, bFind);
  BOOST_TEST(bFind);
  LeafRecord *lr = &priPage->GetRecord(pos);
  BOOST_TEST(lr->GetLock()->_actType == ActionType::UPDATE);
  ReleaseResult rst = lr->ReleaseLock(table->GetVectorIndex()[0]._tree);
  VectorDataValue vct;
  ReadResult result =
      lr->ReadListValue({}, vct, table->GetVectorIndex()[0]._tree);
  BOOST_TEST(result == ReadResult::OK_NOLOCK);
  BOOST_TEST(vct[0]->GetLong() == pval);
  BOOST_TEST(vct[1]->GetLong() == (int32_t)BytesSwap32(unqVal));
  MString ss = GenMString(nonVal);
  BOOST_TEST(ss == (const char *)vct[2]->GetBuff());

  RawKey unqKey({new DataValueInt(BytesSwap32(unqVal))});
  pos = untPage->SearchKey(unqKey, bFind);
  BOOST_TEST(bFind);
  lr = &untPage->GetRecord(pos);
  BOOST_TEST(lr->GetPrimayKey() == priKey);
  ActionType atype =
      (priVal == unqVal ? ActionType::UPDATE : ActionType::INSERT);
  BOOST_TEST(lr->GetLock()->_actType == atype);

  if (priVal != unqVal) {
    RawKey unqKey2({new DataValueInt(BytesSwap32(priVal))});
    pos = untPage->SearchKey(unqKey2, bFind);
    BOOST_TEST(bFind);
    lr = &untPage->GetRecord(pos);
    BOOST_TEST(lr->GetPrimayKey() == priKey);
    BOOST_TEST(lr->GetLock()->_actType == ActionType::DELETE);
  }

  RawKey nonKey({new DataValueVarChar(ss.c_str(), ss.size(), 50)});
  pos = nonPage->SearchKey(nonKey, bFind);
  BOOST_TEST(bFind);
  lr = &nonPage->GetRecord(pos);
  BOOST_TEST(lr->GetPrimayKey() == priKey);
  atype = (priVal == nonVal ? ActionType::UPDATE : ActionType::INSERT);
  BOOST_TEST(lr->GetLock()->_actType == atype);

  if (priVal != nonVal) {
    MString str = GenMString(priVal);
    RawKey nonKey2({new DataValueVarChar(str.c_str(), str.size(), 50)});
    pos = nonPage->SearchKey(nonKey2, bFind);
    BOOST_TEST(bFind);
    lr = &nonPage->GetRecord(pos);
    BOOST_TEST(lr->GetPrimayKey() == priKey);
    BOOST_TEST(lr->GetLock()->_actType == ActionType::DELETE);
  }
}

BOOST_AUTO_TEST_SUITE(StatementTest)
BOOST_AUTO_TEST_CASE(Statement_Point_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  uint16_t tidOld = ThreadPool::SetThreadId(0);
  FilePagePool::Start(8);

  Database *db = new Database(1, ROOT_PATH.c_str(), DB_NAME, MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  PhysTable *table = CreateTable(db, TABLE_NAME);
  TableManager::AddTable(DB_NAME + "." + TABLE_NAME, table);

  ThreadPool *tpool = ThreadPool::CreateMainPool("test", 1, 1);
  tpool->SetStop();
  tpool->WaitStoped();
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
  InsertStatement *stmt = new InsertStatement(StmtId++, TXID_NULL, exprInst,
                                              move(vctRow), &stmtResult);
  session->_lstWaittingStmt.push_back(stmt);
  session->Exec();

  MVector<IndexRange> &vctRange =
      table->GetVectorIndex()[0]._tree->GetVctRange();
  BOOST_TEST(vctRange.size() == 1);
  BOOST_TEST(vctRange[0]._actionQueue->_queueSessionAction.RoughSize() == 100);

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

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(stmt->GetStmtStatus() == StmtStatus::Logging);
  LogTask *logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);
  delete exprInst;

  // Select records with primary key query
  MString msql = "select * from t1 where c1=?";
  VectorRow vctParas;
  int64_t val = (int64_t)vctInt[5] * vctInt[5] + vctInt[5];
  vctParas.push_back({new DataValueLong(val)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  sGroup._threaPoolQueue.Pop(lst);
  assert(lst.size() == 0);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->First());

  VectorDataValue vct;
  BOOST_TEST(stmtResult._resultSet->GetCurrDataValueRow(vct));
  BOOST_TEST(vct[0]->GetLong() == val);
  BOOST_TEST(vct[1]->GetLong() == (int)BytesSwap32(vctInt[5]));
  BOOST_TEST(GenMString(vctInt[5]) == (MString) * (DataValueVarChar *)vct[2]);

  // Select records with unique key query
  msql = "select * from t1 where c2=?";
  vctParas.push_back({new DataValueInt((int)BytesSwap32(vctInt[5]))});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->First());

  vct.clear();
  BOOST_TEST(stmtResult._resultSet->GetCurrDataValueRow(vct));
  BOOST_TEST(vct[0]->GetLong() == val);
  BOOST_TEST(vct[1]->GetLong() == (int)BytesSwap32(vctInt[5]));
  BOOST_TEST(GenMString(vctInt[5]) == (MString) * (DataValueVarChar *)vct[2]);

  // Select records with nonunique key query
  msql = "select * from t1 where c3=?";
  MString ss = GenMString(vctInt[5]);
  vctParas.push_back({new DataValueVarChar(ss.c_str(), ss.size(), 50)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->First());

  vct.clear();
  BOOST_TEST(stmtResult._resultSet->GetCurrDataValueRow(vct));
  BOOST_TEST(vct[0]->GetLong() == val);
  BOOST_TEST(vct[1]->GetLong() == (int)BytesSwap32(vctInt[5]));
  BOOST_TEST(GenMString(vctInt[5]) == (MString) * (DataValueVarChar *)vct[2]);

  // Delete records with primary key
  msql = "delete from t1 where c1=?";
  vctParas.push_back({new DataValueLong(val)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  CheckDeleteRecord(table, vctInt[5], 100);

  // Delete records with unique key
  msql = "delete from t1 where c2=?";
  vctParas.push_back({new DataValueInt((int)BytesSwap32(vctInt[10]))});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  CheckDeleteRecord(table, vctInt[10], 100);

  // Delete records with nonunique key
  msql = "delete from t1 where c3=?";
  ss = GenMString(vctInt[15]);
  vctParas.push_back({new DataValueVarChar(ss.c_str(), ss.size())});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  CheckDeleteRecord(table, vctInt[15], 100);

  // Update records with primary key
  MVector<int> v4 = GenerateInt(124, 4, setInt);
  msql = "update t1 set c2=?,c3=? where c1=?";
  ss = GenMString(v4[1]);
  val = (int64_t)vctInt[20] * vctInt[20] + vctInt[20];
  vctParas.push_back({new DataValueInt(BytesSwap32(v4[0])),
                      new DataValueVarChar(ss.c_str(), ss.size(), 50),
                      new DataValueLong(val)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  CheckUpdateRecord(table, vctInt[20], v4[0], v4[1]);

  // Update records with unique key
  msql = "update t1 set c3=? where c2=?";
  ss = GenMString(v4[2]);
  vctParas.push_back({new DataValueVarChar(ss.c_str(), ss.size(), 50),
                      new DataValueInt((int)BytesSwap32(vctInt[25]))});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  CheckUpdateRecord(table, vctInt[25], vctInt[25], v4[2]);

  // Update records with nonunique key
  msql = "update t1 set c2=? where c3=?";
  ss = GenMString(vctInt[30]);
  vctParas.push_back({new DataValueInt((int)BytesSwap32(v4[3])),
                      new DataValueVarChar(ss.c_str(), ss.size(), 50)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  CheckUpdateRecord(table, vctInt[30], v4[3], vctInt[30]);

  //  Clear
  TableTaskMgr::_dtLastWriteDisk += 10000;
  vctTasks[0][0]->Run();
  vctTasks[0][0]->SetStatus(TaskStatus::FINISHED, false);
  vctTasks[1][0]->Run();
  vctTasks[1][0]->SetStatus(TaskStatus::FINISHED, false);
  vctTasks[2][0]->Run();
  vctTasks[2][0]->SetStatus(TaskStatus::FINISHED, false);

  vector<SessionTask *> &vctSessTask = SessionPool::GetVctSessionTask();
  for (SessionTask *task : vctSessTask) {
    task->SetStatus(TaskStatus::FINISHED, false);
  }

  stmtResult.Reset();
  table->CloseIndex();
  TableManager::ClearTable();
  DatabaseManager::ClearDB();
  CachePagePool::ClearPool();
  ThreadPool::SetThreadId(tidOld);
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  SessionPool::ClearPool();
  LogTask::Clear();
}

BOOST_AUTO_TEST_CASE(Statement_Range_test) {
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
  MVector<int> vctInt = GenerateInt(125, 1000, setInt);
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
  InsertStatement *stmt = new InsertStatement(StmtId++, TXID_NULL, exprInst,
                                              move(vctRow), &stmtResult);
  session->_lstWaittingStmt.push_back(stmt);
  session->Exec();

  IndexActionQueue *iaQueue =
      table->GetVectorIndex()[0]._tree->GetVctRange()[0]._actionQueue;
  BOOST_TEST(iaQueue->_queueSessionAction.RoughSize() == 1000);

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

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(stmt->GetStmtStatus() == StmtStatus::Logging);
  LogTask *logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);
  delete exprInst;

  TableTaskMgr::_dtLastWriteDisk += 10000;
  vctTasks[0][0]->Run();
  vctTasks[1][0]->Run();
  vctTasks[2][0]->Run();

  // Select a range of record with primary key
  MString msql = "select * from t1 where c1<?";
  VectorRow vctParas;
  int64_t val = GetPosInt(setInt, 10);
  vctParas.push_back({new DataValueLong(val * val + val)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  sGroup._threaPoolQueue.Pop(lst);
  assert(lst.size() == 0);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->GetRowCount() == 10);
  BOOST_TEST(stmtResult._resultSet->First());

  auto iter = setInt.begin();
  for (int i = 0; i < 10; i++) {
    VectorDataValue vctDv;
    bool b = stmtResult._resultSet->GetCurrDataValueRow(vctDv);
    BOOST_TEST(b);
    int64_t val = *iter;

    BOOST_TEST(vctDv[0]->GetLong() == val * val + val);
    BOOST_TEST(vctDv[1]->GetLong() == (int32_t)BytesSwap32(*iter));
    MString ss = GenMString(*iter);
    BOOST_TEST(ss == (const char *)vctDv[2]->GetBuff());
    BOOST_TEST(stmtResult._resultSet->Next());
    iter++;
  }

  // Select a range of record with unique key
  MTreeSet<int> setSwap;
  for (int iv : vctInt) {
    setSwap.insert(BytesSwap32(iv));
  }

  int vs = GetPosInt(setSwap, 560);
  int ve = GetPosInt(setSwap, 580);
  msql = "select * from t1 where c2>=? and c2<?";
  vctParas.push_back({new DataValueInt(vs), new DataValueInt(ve)});
  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->GetRowCount() == 20);
  BOOST_TEST(stmtResult._resultSet->First());

  iter = setSwap.find(vs);
  for (int i = 0; i < 20; i++) {
    VectorDataValue vctDv;
    bool b = stmtResult._resultSet->GetCurrDataValueRow(vctDv);
    BOOST_TEST(b);
    int64_t val = (int32_t)BytesSwap32(*iter);
    BOOST_TEST(vctDv[0]->GetLong() == val * val + val);
    BOOST_TEST(vctDv[1]->GetLong() == *iter);
    MString ss = GenMString(val);
    BOOST_TEST(ss == (const char *)vctDv[2]->GetBuff());
    BOOST_TEST(stmtResult._resultSet->Next());
    iter++;
  }

  // Select a range of record with non-unique key
  msql = "select * from t1 where c3>=? and c3<?";
  vs = GetPosInt(setInt, 100);
  ve = GetPosInt(setInt, 120);
  MString ss1 = GenMString(vs);
  MString ss2 = GenMString(ve);
  vctParas.push_back({new DataValueVarChar(ss1.c_str(), ss1.size(), 50),
                      new DataValueVarChar(ss2.c_str(), ss2.size(), 50)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->GetRowCount() == 20);
  BOOST_TEST(stmtResult._resultSet->First());

  iter = setInt.find(vs);
  for (int i = 0; i < 20; i++) {
    VectorDataValue vctDv;
    bool b = stmtResult._resultSet->GetCurrDataValueRow(vctDv);
    BOOST_TEST(b);
    int64_t val = *iter;

    BOOST_TEST(vctDv[0]->GetLong() == val * val + val);
    BOOST_TEST(vctDv[1]->GetLong() == (int32_t)BytesSwap32(*iter));
    MString ss = GenMString(*iter);
    BOOST_TEST(ss == (const char *)vctDv[2]->GetBuff());
    BOOST_TEST(stmtResult._resultSet->Next());
    iter++;
  }

  // Update with multi fileds
  msql = "update t1 set c2=c2+1 where c1>=? and c1<? and c3>=? and c3<?";
  int64_t lvs = GetPosInt(setInt, 100);
  lvs = lvs * lvs + lvs;
  int64_t lve = GetPosInt(setInt, 120);
  lve = lve * lve + lve;
  ss1 = GenMString(GetPosInt(setInt, 110));
  ss2 = GenMString(GetPosInt(setInt, 130));
  vctParas.push_back({new DataValueLong(lvs), new DataValueLong(lve),
                      new DataValueVarChar(ss1.c_str(), ss1.size(), 50),
                      new DataValueVarChar(ss2.c_str(), ss2.size(), 50)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(session->_currStatement->GetStmtStatus() == StmtStatus::Logging);
  logTask = LogTask::GetTask();
  s = logTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
#endif

  BOOST_TEST(session->_currStatement == nullptr);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);

  // Select with multi fileds
  msql = "select * from t1 where c1>=? and c1<?";
  lvs = GetPosInt(setInt, 100);
  lvs = lvs * lvs + lvs;
  lve = GetPosInt(setInt, 130);
  lve = lve * lve + lve;
  vctParas.push_back({new DataValueLong(lvs), new DataValueLong(lve)});

  SessionPool::AddStatement(0, 0, StmtId++, ExprId++, move(msql),
                            move(vctParas), &stmtResult);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = sessTask->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);
  BOOST_TEST(stmtResult._resultSet->GetRowCount() == 30);
  BOOST_TEST(stmtResult._resultSet->First());

  iter = setInt.find(GetPosInt(setInt, 100));
  for (int i = 0; i < 30; i++) {
    VectorDataValue vctDv;
    bool b = stmtResult._resultSet->GetCurrDataValueRow(vctDv);
    BOOST_TEST(b);
    int64_t val = *iter;

    BOOST_TEST(vctDv[0]->GetLong() == val * val + val);
    if (i >= 10 && i < 20) {
      BOOST_TEST(vctDv[1]->GetLong() == (int32_t)BytesSwap32(*iter) + 1);
    } else {
      BOOST_TEST(vctDv[1]->GetLong() == (int32_t)BytesSwap32(*iter));
    }
    MString ss = GenMString(*iter);
    BOOST_TEST(ss == (const char *)vctDv[2]->GetBuff());
    BOOST_TEST(stmtResult._resultSet->Next());
    iter++;
  }

  val = GetPosInt(setInt, 115);
  VectorDataValue vdv = {new DataValueInt(BytesSwap32(val) + 1)};
  RawKey ukey(vdv);
  IndexTree *unqTree = table->GetVectorIndex()[1]._tree;
  IndexPage *idxPage = unqTree->GetRootPage();
  bool bFind = unqTree->SearchPage(ukey, idxPage);
  BOOST_TEST(bFind);
  LeafPage *lpage = dynamic_cast<LeafPage *>(idxPage);
  int pos = lpage->SearchKey(ukey, bFind);
  BOOST_TEST(bFind);

  RawKey pkey({new DataValueLong(val * val + val)});
  LeafRecord &lr = lpage->GetRecord(pos);
  BOOST_TEST(pkey == lr.GetPrimayKey());

  //  Clear
  TableTaskMgr::_dtLastWriteDisk += 10000;
  vctTasks[0][0]->Run();
  vctTasks[0][0]->SetStatus(TaskStatus::FINISHED, false);
  vctTasks[1][0]->Run();
  vctTasks[1][0]->SetStatus(TaskStatus::FINISHED, false);
  vctTasks[2][0]->Run();
  vctTasks[2][0]->SetStatus(TaskStatus::FINISHED, false);

  vector<SessionTask *> &vctSessTask = SessionPool::GetVctSessionTask();
  for (SessionTask *task : vctSessTask) {
    task->SetStatus(TaskStatus::FINISHED, false);
  }

  stmtResult.Reset();
  table->CloseIndex();
  TableManager::ClearTable();
  DatabaseManager::ClearDB();
  CachePagePool::ClearPool();
  ThreadPool::SetThreadId(tidOld);
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  SessionPool::ClearPool();
  LogTask::Clear();
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
