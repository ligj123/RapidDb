#include "../../src/table/TableTaskMgr.h"
#include "../../src/binlog/LogTask.h"
#include "../../src/core/BranchPage.h"
#include "../../src/core/BranchRecord.h"
#include "../../src/core/IndexTree.h"
#include "../../src/core/LeafPage.h"
#include "../../src/dataType/DataValueFactory.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/manager/DatabaseManager.h"
#include "../../src/manager/TableManager.h"
#include "../../src/pool/CachePagePool.h"
#include "../../src/pool/FilePagePool.h"
#include "../../src/serv/SessionPool.h"
#include "../../src/statement/InsertStatement.h"
#include "../../src/table/Column.h"
#include "../../src/table/Database.h"
#include "../../src/table/Table.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/Utilitys.h"
#include "../TestHeader.h"

#include <boost/test/unit_test.hpp>

namespace storage {
static uint32_t stmtId{0};
static TranID txID{0};

ExprInsert *CreateInsertExpression(Database *db, const MString &tableName) {
  PhysTable *ptable =
      new PhysTable(db, tableName, 0x100, MilliSecTime(), MilliSecTime());
  ptable->AddColumn("c1", DataType::FIXCHAR, false, 1000, "primary key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c2", DataType::VARCHAR, false, 1000, "Unique Key",
                    Charsets::UTF8, nullptr);
  ptable->AddColumn("c3", DataType::FIXCHAR, true, 50, "NonUnique Key",
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

  ExprInsert *exprInst = new ExprInsert;
  exprInst->_exprTable = new ExprTable(new MString(db->GetDbName()),
                                       new MString(tableName), nullptr);

  exprInst->_vctCol = new MVectorPtr<ExprColumn *>();
  exprInst->_vctCol->reserve(ptable->GetColumnArray().size());

  for (const PhysColumn &pcol : ptable->GetColumnArray()) {
    ExprColumn *ecol =
        new ExprColumn(new MString(pcol.GetName()), nullptr, nullptr);
    ecol->_pos = pcol.GetIndex();
    ecol->_dataType = pcol.GetDataType();
    ecol->_dataLength = pcol.GetMaxLength();
    exprInst->_vctCol->push_back(ecol);
  }

  MVectorPtr<ExprElem *> *vctElem = new MVectorPtr<ExprElem *>();
  for (size_t i = 0; i < ptable->GetColumnArray().size(); i++) {
    ExprParameter *epara = new ExprParameter();
    epara->_paraPos = (int)i;
    vctElem->push_back(epara);
    exprInst->_vctPara.push_back(epara);
  }
  exprInst->_vctRowData = new MVectorPtr<MVectorPtr<ExprElem *> *>();
  exprInst->_vctRowData->push_back(vctElem);
  exprInst->_exprTable->_physTable = ptable;
  return exprInst;
}

MVector<int> GenerateInt(int num, MTreeSet<int> &mset) {
  std::srand(123456);
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

VectorRow GenInsertRecords(const MVector<int> &mvct) {
  MString fix1000 = "FIXCHAR_1000_" + MString(987, 'a');
  MString var1000 = "VARCHAR_1000_" + MString(987, 'a');
  MString fix50 = "FIXCHAR_50_" + MString(37, 'a');

  VectorRow vctRow;
  vctRow.reserve(mvct.size());

  for (int ival : mvct) {
    VectorDataValue vctDv;
    DataValueFixChar *dvFix1000 =
        new DataValueFixChar(fix1000.c_str(), 999, 1000);
    DataValueVarChar *dvVar1000 =
        new DataValueVarChar(var1000.c_str(), 999, 1000);
    DataValueFixChar *dvFix50 = new DataValueFixChar(fix50.c_str(), 49, 50);

    stringstream ss;
    ss << "_0x" << std::setfill('0') << std::setw(8) << std::hex << ival;

    memcpy(const_cast<Byte *>(dvFix1000->GetBuff()) + 988, ss.str().c_str(),
           11);
    memcpy(const_cast<Byte *>(dvVar1000->GetBuff()) + 988, ss.str().c_str(),
           11);
    memcpy(const_cast<Byte *>(dvFix50->GetBuff()) + 38, ss.str().c_str(), 11);
    vctDv.push_back(dvFix1000);
    vctDv.push_back(dvVar1000);
    vctDv.push_back(dvFix50);
    vctRow.push_back(move(vctDv));
  }

  return vctRow;
}

BOOST_AUTO_TEST_SUITE(TableTest)

BOOST_AUTO_TEST_CASE(TableTaskMgr_test) {
  LOG_INFO << "Run testcase: "
           << boost::unit_test::framework::current_test_case().p_name;
  const MString TABLE_NAME = "testTableMgr";
  const MString DB_NAME = "testDbMgr";
  uint16_t tidOld = ThreadPool::SetThreadId(0);
  FilePagePool::Start(8);

  Database *db = new Database(1, ROOT_PATH.c_str(), DB_NAME, MilliSecTime(),
                              MicroSecTime());
  DatabaseManager::AddDb(db);
  ExprInsert *exprInsert = CreateInsertExpression(db, TABLE_NAME);
  PhysTable *table = exprInsert->_exprTable->_physTable;

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

  MTreeSet<int> mset;
  MVector<int> vctInt = GenerateInt(500, mset);
  VectorRow vctRow = GenInsertRecords(vctInt);

  uint32_t sid = 0;
  SessionGroup &sGroup = SessionPool::GetVctSessionGroup()[0];
  Session *session = new Session(sid);
  sGroup._mapSession.emplace(sid, session);
  StmtResult stmtResult;
  InsertStatement *stmt = new InsertStatement(stmtId++, TXID_NULL, exprInsert,
                                              move(vctRow), &stmtResult);
  session->_lstWaittingStmt.push_back(stmt);
  session->Exec();

  BOOST_TEST(table->GetVectorIndex()[0]
                 ._tree->GetVctRange()[0]
                 ._actionQueue->_queueSessionAction.RoughSize() == 500);
  BOOST_TEST(table->GetVectorIndex()[1]
                 ._tree->GetVctRange()[0]
                 ._actionQueue->_queueSessionAction.RoughSize() == 0);
  BOOST_TEST(table->GetVectorIndex()[2]
                 ._tree->GetVctRange()[0]
                 ._actionQueue->_queueSessionAction.RoughSize() == 0);

  MVector<MVector<IndexTask *>> &vctTasks = tmgr->GetVctIndexTasks();
  BOOST_TEST(vctTasks.size() == 3);
  BOOST_TEST(vctTasks[0].size() == 1);
  BOOST_TEST(vctTasks[1].size() == 1);
  BOOST_TEST(vctTasks[2].size() == 1);

  TaskStatus s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);
  SecIndexActionQueue *secQueue = dynamic_cast<SecIndexActionQueue *>(
      table->GetVectorIndex()[1]._tree->GetVctRange()[0]._actionQueue);
  BOOST_TEST(secQueue->_fromPrimaryQueue.RoughSize() == 500);
  secQueue = dynamic_cast<SecIndexActionQueue *>(
      table->GetVectorIndex()[2]._tree->GetVctRange()[0]._actionQueue);
  BOOST_TEST(secQueue->_fromPrimaryQueue.RoughSize() == 500);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  MList<SessionAction *> lst;
  sGroup._threaPoolQueue.Pop(lst);
  for (auto iter = lst.begin(); iter != lst.end();) {
    TaskStatus s = (*iter)->Exec(sGroup);
    BOOST_TEST(s == TaskStatus::FINISHED);
    delete (*iter);
    iter = lst.erase(iter);
  }

  assert(lst.size() == 0);

  session->Exec();
#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(stmt->GetStmtStatus() == StmtStatus::Logging);
  session->_transaction.SetLogged();
  session->Exec();
#endif
  BOOST_TEST(session->_currStatement == nullptr);

  TableTaskMgr::_dtLastWriteDisk += 10000;

  for (int i = 0; i < 3; i++) {
    s = vctTasks[i][0]->Run();
    BOOST_TEST(s == TaskStatus::INTERVAL);

    IndexTree *tree = vctProp[i]._tree;
    BOOST_TEST(!tree->IsMultiRange());
    IndexRange &range = tree->GetVctRange()[0];
    BOOST_TEST(range._actionQueue->_lstAction.size() == 0);
    BOOST_TEST(range._pageMap.size() == 1);
  }

  BOOST_TEST(stmtResult._rowNum == 500);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);

  vctInt = GenerateInt(500, mset);
  vctRow = GenInsertRecords(vctInt);
  stmtResult._rowNum = 0;
  stmtResult._status.store(ResultStatus::INIT, memory_order_relaxed);
  stmt = new InsertStatement(stmtId++, TXID_NULL, exprInsert, move(vctRow),
                             &stmtResult);

  session->_lstWaittingStmt.push_back(stmt);
  session->Exec();
  tmgr->CollectTaskData(0, 0);
  vctTasks[0][0]->SetStatus(TaskStatus::FINISHED, false);

  IndexAdjustTask *adjustTask = new IndexAdjustTask(tpool, tmgr, 0, 5);
  adjustTask->Run();
  adjustTask->Run();
  delete adjustTask;

  BOOST_TEST(vctTasks[0].size() == 5);
  MVector<storage::IndexRange> &vctRange = vctProp[0]._tree->GetVctRange();

  size_t count = 0;
  for (IndexRange &range : vctRange) {
    count += range._actionQueue->_lstAction.size();
  }
  BOOST_TEST(count == 500);

  for (IndexTask *task : vctTasks[0]) {
    s = task->Run();
    BOOST_TEST(s == TaskStatus::INTERVAL);
  }

  secQueue = dynamic_cast<SecIndexActionQueue *>(
      table->GetVectorIndex()[1]._tree->GetVctRange()[0]._actionQueue);
  BOOST_TEST(secQueue->_fromPrimaryQueue.RoughSize() == 500);
  secQueue = dynamic_cast<SecIndexActionQueue *>(
      table->GetVectorIndex()[2]._tree->GetVctRange()[0]._actionQueue);
  BOOST_TEST(secQueue->_fromPrimaryQueue.RoughSize() == 500);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  sGroup._threaPoolQueue.Pop(lst);
  for (auto iter = lst.begin(); iter != lst.end();) {
    TaskStatus s = (*iter)->Exec(sGroup);
    assert(s == TaskStatus::FINISHED);
    delete (*iter);
    iter = lst.erase(iter);
  }

  BOOST_TEST(lst.size() == 0);

  session->Exec();
#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(stmt->GetStmtStatus() == StmtStatus::Logging);
  session->_transaction.SetLogged();
  session->Exec();
#endif
  BOOST_TEST(session->_currStatement == nullptr);

  TableTaskMgr::_dtLastWriteDisk += 10000;
  IndexTree *tree = vctProp[0]._tree;
  BOOST_TEST(tree->IsMultiRange());
  for (size_t i = 0; i < vctTasks[0].size(); i++) {
    IndexTask *task = vctTasks[0][i];
    s = task->Run();
    BOOST_TEST(s == TaskStatus::INTERVAL);

    IndexRange &range = tree->GetVctRange()[i];

    BOOST_TEST(range._actionQueue->_lstAction.size() == 0);
    BOOST_TEST(range._pageMap.size() == (i == 0 ? 1 : 0));
  }

  for (int i = 1; i < 3; i++) {
    IndexTask *task = vctTasks[i][0];
    s = task->Run();
    BOOST_TEST(s == TaskStatus::INTERVAL);

    IndexTree *tree = vctProp[i]._tree;
    IndexRange &range = tree->GetVctRange()[0];
    BOOST_TEST(range._actionQueue->_lstAction.size() == 0);
    BOOST_TEST(range._pageMap.size() == 1);
  }

  BOOST_TEST(stmtResult._rowNum == 500);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);

  // From 5 tasks to 1 task
  vctInt = GenerateInt(500, mset);
  vctRow = GenInsertRecords(vctInt);
  stmtResult._rowNum = 0;
  stmtResult._status.store(ResultStatus::INIT, memory_order_relaxed);
  stmt = new InsertStatement(stmtId++, TXID_NULL, exprInsert, move(vctRow),
                             &stmtResult);

  session->_lstWaittingStmt.push_back(stmt);
  session->Exec();

  for (size_t i = 0; i < vctTasks[0].size(); i++) {
    tmgr->CollectTaskData(0, i);
    vctTasks[0][i]->SetStatus(TaskStatus::FINISHED, false);
  }

  adjustTask = new IndexAdjustTask(tpool, tmgr, 0, 1);
  adjustTask->Run();
  adjustTask->Run();
  delete adjustTask;

  BOOST_TEST(vctTasks[0].size() == 1);
  BOOST_TEST(
      vctProp[0]._tree->GetVctRange()[0]._actionQueue->_lstAction.size() ==
      500);

  s = vctTasks[0][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  secQueue = dynamic_cast<SecIndexActionQueue *>(
      table->GetVectorIndex()[1]._tree->GetVctRange()[0]._actionQueue);
  BOOST_TEST(secQueue->_fromPrimaryQueue.RoughSize() == 500);
  secQueue = dynamic_cast<SecIndexActionQueue *>(
      table->GetVectorIndex()[2]._tree->GetVctRange()[0]._actionQueue);
  BOOST_TEST(secQueue->_fromPrimaryQueue.RoughSize() == 500);

  s = vctTasks[1][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  s = vctTasks[2][0]->Run();
  BOOST_TEST(s == TaskStatus::INTERVAL);

  sGroup._threaPoolQueue.Pop(lst);
  BOOST_TEST(lst.size() == 500);

  for (auto iter = lst.begin(); iter != lst.end();) {
    TaskStatus s = (*iter)->Exec(sGroup);
    assert(s == TaskStatus::FINISHED);
    delete (*iter);
    iter = lst.erase(iter);
  }

  assert(lst.size() == 0);

  session->Exec();
#ifndef WITHOUT_BIN_LOG
  BOOST_TEST(stmt->GetStmtStatus() == StmtStatus::Logging);
  session->_transaction.SetLogged();
  session->Exec();
#endif
  BOOST_TEST(session->_currStatement == nullptr);

  TableTaskMgr::_dtLastWriteDisk += 10000;
  tree = vctProp[0]._tree;
  BOOST_TEST(!tree->IsMultiRange());

  for (int i = 0; i < 3; i++) {
    IndexTask *task = vctTasks[i][0];
    s = task->Run();
    BOOST_TEST(s == TaskStatus::INTERVAL);

    IndexTree *tree = vctProp[i]._tree;
    IndexRange &range = tree->GetVctRange()[0];
    BOOST_TEST(range._actionQueue->_lstAction.size() == 0);
    BOOST_TEST(range._pageMap.size() == 1);
  }

  BOOST_TEST(stmtResult._rowNum == 500);
  BOOST_TEST(stmtResult._status.load(memory_order_relaxed) ==
             ResultStatus::FINISHED);

  for (int i = 0; i < 3; i++) {
    Byte level = 255;
    RawRecord *rrPrev = nullptr;

    MList<IndexPage *> lst;
    lst.push_back(vctProp[i]._tree->GetRootPage());

    while (lst.size() > 0) {
      IndexPage *idxPage = lst.front();
      lst.pop_front();

      if (idxPage->GetPageLevel() != level) {
        level = idxPage->GetPageLevel();
        rrPrev = nullptr;
      }

      for (RawRecord *rr : idxPage->GetRecords()) {
        if (rrPrev == nullptr) {
          rrPrev = rr;
        }

        if (idxPage->GetPageType() == PageType::BRANCH_PAGE) {
          BranchRecord *br = (BranchRecord *)rr;
          lst.push_back(br->GetChildPage());
          if (br->CompareTo(*rrPrev) < 0) {
            BOOST_TEST(br->CompareTo(*rrPrev) >= 0);
          }
        } else {
          LeafRecord *lr = (LeafRecord *)rr;
          if (lr->CompareTo(*(LeafRecord *)rrPrev) < 0) {
            BOOST_TEST(lr->CompareTo(*(LeafRecord *)rrPrev) >= 0);
          }
        }

        rrPrev = rr;
      }
    }
  }

  for (MVector<IndexTask *> &vctTask : vctTasks) {
    for (IndexTask *task : vctTask) {
      task->SetStatus(TaskStatus::FINISHED, false);
    }
  }

  vector<SessionTask *> &vctSessTask = SessionPool::GetVctSessionTask();
  for (SessionTask *task : vctSessTask) {
    task->SetStatus(TaskStatus::FINISHED, false);
  }

  table->CloseIndex();
  delete table;
  delete exprInsert;

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
