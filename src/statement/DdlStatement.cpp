#include "DdlStatement.h"
#include "../binlog/LogTask.h"
#include "../core/LeafPage.h"
#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../sysTable/SysTable.h"
#include "../table/TableTaskMgr.h"

#include <filesystem>

namespace fs = std::filesystem;
namespace storage {
StmtStatus StmtCreateDatabase::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    MString *dbName = GetExprStatement()->_dbName;
    Database *db = DatabaseManager::FindDb(*dbName);
    if (db != nullptr) {
      _threadErrorMsg.reset(new ErrorMsg(DDL_DATABASE_EXIST, {*dbName}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      return StmtStatus::Finished;
    }

    PhysTable *dbTable = nullptr;
    bool b = TableManager::FindTable(SysTable::SystemDbTableName(), dbTable);
    assert(b && dbTable != nullptr);

    string path =
        Configure::GetDbRootPath() + "/" + string(dbName->c_str()) + "_";
    int ii = 1;
    MString folder;

    while (true) {
      if (!fs::exists(path + to_string(ii))) {
        folder = *dbName + "_" + ToMString(ii);
        break;
      }

      ii++;
    }

    b = fs::create_directories(path);
    if (!b) {
      _threadErrorMsg.reset(
          new ErrorMsg(DDL_DATABASE_CREATE_FAILED, {*dbName}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      return StmtStatus::Finished;
    }

    VectorDataValue vdv;
    vdv.reserve(5);
    vdv.push_back(nullptr);

    vdv.push_back(new DataValueVarChar(dbName->c_str(),
                                       static_cast<uint32_t>(dbName->size())));

    vdv.push_back(new DataValueVarChar(folder.c_str(),
                                       static_cast<uint32_t>(folder.size())));
    vdv.push_back(new DataValueDateTime(MilliSecTime()));
    vdv.push_back(nullptr);

    VectorDataValue vctKey;
    vctKey.push_back(new DataValueInt());

    _instRecord =
        new StmtInsertRecord(RawKey(vctKey), move(vdv), this, dbTable);
    StmtInsertAction *action =
        new StmtInsertAction(dbTable->GetIndexTree(0), _instRecord);
    dbTable->GetTableTaskMgr()->AddSessionAction(0, GetSessionGroupId(),
                                                 action);
    _status = StmtStatus::Executing;
    return _status;
  } else if (_status == StmtStatus::Executing) {
    if (_instRecord != nullptr) {
      ActionStatus s = _instRecord->_status.load(memory_order_acquire);
      if (s == ActionStatus::INIT) {
        return _status;
      }

      VectorDataValue &vdv = _instRecord->_vctParas;
      _db = new Database(
          (int)(*dynamic_cast<DataValueInt *>(vdv[0])),
          (MString)(*dynamic_cast<DataValueVarChar *>(vdv[2])),
          (MString)(*dynamic_cast<DataValueVarChar *>(vdv[1])),
          (DT_MicroSec)(*dynamic_cast<DataValueDateTime *>(vdv[3])),
          (DT_MicroSec)(*dynamic_cast<DataValueDateTime *>(vdv[4])));
      delete _instRecord;
      _instRecord = nullptr;
    }

    if (_lstWaitRecord.size() > 0) {
      assert(_lstWaitRecord.size() == 1);
      RecordResult res = _lstWaitRecord.back()->GetLock()->GetRecordResult();
      if (res == RecordResult::INIT) {
        return _status;
      }

      if (res == RecordResult::ERROR) {
        _stmtResult->_vctError.push_back(
            move(_lstWaitRecord.back()->GetLock()->_errMsg->GetErrorMsg()));
        _stmtFailed.store(true, memory_order_relaxed);
      }

      _lstFinishRecord.push_back(_lstWaitRecord.back());
      _lstWaitRecord.clear();
    }

    if (_stmtFailed.load(memory_order_relaxed)) {
      _lstFinishRecord.back()->GetLock()->_recStatus.store(
          RecordStatus::ROLLBACKED, memory_order_relaxed);

      _stmtResult->_bFailed = true;
      _stmtResult->_rowNum = 0;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;

      fs::path path = _db->GetDbPath();
      fs::remove(path);
      return _status;
    }

    assert(sess->_transaction.IsAutoCommit());
#ifdef WITHOUT_BIN_LOG
    _status = StmtStatus::Finished;
  }
#else
    _status = StmtStatus::Logging;
    LogTask::AddTransaction(ThreadPool::GetThreadId(), &(sess->_transaction));
    return _status;
  } else if (_status == StmtStatus::Logging) {
    if (sess->_transaction.IsLogged()) {
      _status = StmtStatus::Finished;
    } else {
      return _status;
    }
  }
#endif

  _lstFinishRecord.back()->GetLock()->_recStatus.store(RecordStatus::COMMITED,
                                                       memory_order_relaxed);

  _stmtResult->_rowNum = 1;
  _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  _status = StmtStatus::Finished;
  DatabaseManager::AddDb(_db);
  _db = nullptr;

  return StmtStatus::Finished;
}

StmtStatus StmtDropDatabase::SessionExec(Session *sess) {
  // if (_status == StmtStatus::Created) {
  //   assert(_midVar == nullptr);
  //   _midVar = new MiddleVar();

  //   ExprDropDatabase *expr = GetExprStatement();
  //   Database *db = DatabaseManager::FindDb(*expr->_dbName);
  //   if (db == nullptr) {
  //     if (!expr->_ifExist) {
  //       _threadErrorMsg.reset(
  //           new ErrorMsg(DDL_DATABASE_NOT_EXIST, {*expr->_dbName}));
  //       _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
  //       SetStmtFailed(true);
  //     }

  //     _stmtResult->_rowNum = 0;
  //     SetFinished(true);
  //     return StmtStatus::Finished;
  //   }

  //   PhysTable *dbTable = nullptr;
  //   bool b = TableManager::FindTable(SysTable::SystemDbTableName(), dbTable);
  //   assert(b && dbTable != nullptr);

  // }

  return StmtStatus::Finished;
}

StmtStatus StmtShowDatabases::SessionExec(Session *sess) {
  assert(_status == StmtStatus::Created);

  MVector<MString> vct;
  DatabaseManager::ListDb(vct);

  _stmtResult->_resultSet = new CacheResultSet(&GetExprStatement()->_vctCol);
  for (size_t i = 0; i < vct.size(); i++) {
    VectorDataValue vdv;
    vdv.push_back(new DataValueVarChar(vct[i].c_str(), vct[i].size()));
    _stmtResult->_resultSet->AddRow(move(vdv));
  }

  _stmtResult->_rowNum = vct.size();
  _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  _status = StmtStatus::Finished;
  return StmtStatus::Finished;
}

StmtStatus StmtUseDatabase::SessionExec(Session *sess) {
  assert(_status == StmtStatus::Created);
  ExprUseDatabase *expr = GetExprStatement();
  Database *db = DatabaseManager::FindDb(*expr->_dbName);
  if (db == nullptr) {
    _threadErrorMsg.reset(
        new ErrorMsg(DDL_DATABASE_NOT_EXIST, {*expr->_dbName}));
    _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
    SetStmtFailed(true);
    return StmtStatus::Finished;
  }

  sess->_currDb = db;
  _stmtResult->_rowNum = 0;
  _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  _status = StmtStatus::Finished;
  return StmtStatus::Finished;
}

StmtStatus StmtCreateTable::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    ExprCreateTable *expr = GetExprStatement();
    assert(expr->_table->_db != nullptr);
    MString fname = *expr->_table->_dbName + "." + *expr->_table->_tName;
    PhysTable *newTbl = nullptr;
    bool b = TableManager::FindTable(fname, newTbl);
    if (b) {
      _threadErrorMsg.reset(new ErrorMsg(DDL_TABLE_EXIST, {fname}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      _status = StmtStatus::Finished;
      return StmtStatus::Finished;
    }

    PhysTable *tblTable = nullptr;
    b = TableManager::FindTable(SysTable::SystemTblTableName(), tblTable);
    assert(b);

    MString spath =
        expr->_table->_db->GetDbPath() + "/" + *expr->_table->_tName + "_";
    int ii = 1;

    while (true) {
      MString ss = spath + ToMString(ii);
      if (!fs::exists(ss)) {
        spath = move(ss);
        break;
      }

      ii++;
    }

    b = fs::create_directories(spath);
    if (!b) {
      _threadErrorMsg.reset(new ErrorMsg(DDL_TABLE_CREATE_FAILED, {spath}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      return StmtStatus::Finished;
    }

    VectorDataValue vdv;
    vdv.reserve(7);
    vdv.push_back(nullptr);
    vdv.push_back(new DataValueVarChar(expr->_table->_dbName->c_str(),
                                       expr->_table->_dbName->size()));
    vdv.push_back(new DataValueVarChar(expr->_table->_tName->c_str(),
                                       expr->_table->_tName->size()));
    vdv.push_back(new DataValueVarChar(spath.c_str(), spath.size()));
    uint32_t sz = expr->_physTable->CalcSize();
    Byte *tmp = CachePool::Apply(sz);
    expr->_physTable->SaveData(tmp);
    vdv.push_back(new DataValueBlob(sz, tmp));
    vdv.push_back(new DataValueDateTime(expr->_physTable->GetCreateTime()));
    vdv.push_back(new DataValueDateTime(expr->_physTable->GetLastUpdateTime()));

    VectorDataValue vctKey;
    vctKey.push_back(new DataValueInt());
    _instRecord =
        new StmtInsertRecord(RawKey(vctKey), move(vdv), this, tblTable);
    StmtInsertAction *action =
        new StmtInsertAction(tblTable->GetIndexTree(0), _instRecord);
    tblTable->GetTableTaskMgr()->AddSessionAction(0, GetSessionGroupId(),
                                                  action);
    _status = StmtStatus::Executing;
    return _status;
  } else if (_status == StmtStatus::Executing) {
    if (_instRecord != nullptr) {
      ActionStatus s = _instRecord->_status.load(memory_order_acquire);
      if (s == ActionStatus::INIT) {
        return _status;
      }

      VectorDataValue &vdv = _instRecord->_vctParas;
      ExprCreateTable *expr = GetExprStatement();
      expr->_physTable->SetID((int)(*dynamic_cast<DataValueInt *>(vdv[0])));

      delete _instRecord;
      _instRecord = nullptr;
    }

    if (_lstWaitRecord.size() > 0) {
      assert(_lstWaitRecord.size() == 1);
      RecordResult res = _lstWaitRecord.back()->GetLock()->GetRecordResult();
      if (res == RecordResult::INIT) {
        return _status;
      }

      if (res == RecordResult::ERROR) {
        _stmtResult->_vctError.push_back(
            move(_lstWaitRecord.back()->GetLock()->_errMsg->GetErrorMsg()));
        _stmtFailed.store(true, memory_order_relaxed);
      }

      _lstFinishRecord.push_back(_lstWaitRecord.back());
      _lstWaitRecord.clear();
    }

    if (_stmtFailed.load(memory_order_relaxed)) {
      _lstFinishRecord.back()->GetLock()->_recStatus.store(
          RecordStatus::ROLLBACKED, memory_order_relaxed);

      _stmtResult->_bFailed = true;
      _stmtResult->_rowNum = 0;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;

      ExprCreateTable *expr = GetExprStatement();
      MString spath =
          expr->_table->_db->GetDbPath() + "/" + expr->_physTable->GetPath();
      fs::remove(spath.c_str());

      return _status;
    }

    assert(sess->_transaction.IsAutoCommit());
#ifdef WITHOUT_BIN_LOG
    _status = StmtStatus::Finished;
  }
#else
    _status = StmtStatus::Logging;
    LogTask::AddTransaction(ThreadPool::GetThreadId(), &(sess->_transaction));
    return _status;
  } else if (_status == StmtStatus::Logging) {
    if (sess->_transaction.IsLogged()) {
      _status = StmtStatus::Finished;
    } else {
      return _status;
    }
  }
#endif

  ExprCreateTable *expr = GetExprStatement();
  bool b = expr->_physTable->CreateTable();
  assert(b);
  TableManager::AddTable(expr->_physTable);
  expr->_physTable = nullptr;

  _lstFinishRecord.back()->GetLock()->_recStatus.store(RecordStatus::COMMITED,
                                                       memory_order_relaxed);
  _stmtResult->_rowNum = 1;
  _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  _status = StmtStatus::Finished;

  return StmtStatus::Finished;
}

StmtStatus StmtDropTable::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    ExprDropTable *expr = GetExprStatement();
    assert(expr->_table->_db != nullptr);
    MString fname = *expr->_table->_dbName + "." + *expr->_table->_tName;
    PhysTable *delTbl = nullptr;
    bool b = TableManager::FindTable(fname, delTbl);
    if (!b) {
      _threadErrorMsg.reset(new ErrorMsg(DDL_TABLE_NOT_EXIST, {fname}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      _status = StmtStatus::Finished;
      return StmtStatus::Finished;
    }

    PhysTable *sysTable = nullptr;
    b = TableManager::FindTable(SysTable::SystemTblTableName(), sysTable);
    assert(b);
    _midVar = new MiddleVar();
    _midVar->_table = sysTable;
    _midVar->_indexPos = 0;
    IndexProp &prop = sysTable->GetVectorIndex()[0];

    VectorDataValue vctDv;
    vctDv.push_back(new DataValueInt(delTbl->TableID()));
    RawKey *sKey = new RawKey(vctDv);
    _midVar->_vctKeyRange.emplace_back(sKey, nullptr, false, true, true);

    StatementAction *action = new StatementAction(prop._tree, this);
    TableTaskMgr *mgr = sysTable->GetTableTaskMgr();
    mgr->AddSessionAction(0, GetSessionGroupId(), action);
    _status = StmtStatus::Executing;
  } else if (_status == StmtStatus::Executing) {
    if (!_bFinished.load(memory_order_acquire)) {
      return _status;
    }

    if (_lstWaitRecord.size() > 0) {
      assert(_lstWaitRecord.size() == 1);

      RecordResult res = _lstWaitRecord.back()->GetLock()->GetRecordResult();
      if (res == RecordResult::INIT) {
        return _status;
      }

      if (res == RecordResult::ERROR) {
        _stmtResult->_vctError.push_back(
            move(_lstWaitRecord.back()->GetLock()->_errMsg->GetErrorMsg()));
        _stmtFailed.store(true, memory_order_relaxed);
      }

      _lstFinishRecord.push_back(_lstWaitRecord.back());
      _lstWaitRecord.clear();
    }

    if (_stmtFailed.load(memory_order_relaxed)) {
      _lstFinishRecord.back()->GetLock()->_recStatus.store(
          RecordStatus::ROLLBACKED, memory_order_relaxed);

      _stmtResult->_bFailed = true;
      _stmtResult->_rowNum = 0;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
      return _status;
    }

    assert(sess->_transaction.IsAutoCommit());
#ifdef WITHOUT_BIN_LOG
    _status = StmtStatus::Finished;
  }
#else
    _status = StmtStatus::Logging;
    LogTask::AddTransaction(ThreadPool::GetThreadId(), &(sess->_transaction));
    return _status;
  } else if (_status == StmtStatus::Logging) {
    if (sess->_transaction.IsLogged()) {
      _status = StmtStatus::Finished;
    } else {
      return _status;
    }
  }
#endif

  ExprDropTable *expr = GetExprStatement();
  MString fname = *expr->_table->_dbName + "." + *expr->_table->_tName;
  TableManager::RemoveTable(fname);

  _lstFinishRecord.back()->GetLock()->_recStatus.store(RecordStatus::COMMITED,
                                                       memory_order_relaxed);
  _stmtResult->_rowNum = 1;
  _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  _status = StmtStatus::Finished;

  return StmtStatus::Finished;
}

TriBool StmtDropTable::HandleLeafRecord(LeafPage *page, int pagePos,
                                        int rangePos,
                                        VectorLeafRecord *vctLeafRec) {

  ExprDropTable *exprDrop = GetExprStatement();
  PhysTable *table = exprDrop->_table->_physTable;
  MVector<IndexProp> &vctProp = table->GetVectorIndex();

  LeafRecord *lr = &page->GetRecord(pagePos);
  if (lr->ReleaseLockAble()) {
    int32_t commLen1, commLen2, tempLen1, tempLen2;
    lr->GetLength(tempLen1, commLen1);
    lr->ReleaseLock(page->GetIndexTree());
    lr->GetLength(tempLen2, commLen2);
    page->UpdateDataLength(commLen2 - commLen1, tempLen2 - tempLen1);
  }

  if (lr->GetLock() != nullptr) {
    _threadErrorMsg.reset(new ErrorMsg(STMT_LOCK_CONFLICT, {}));
    SendErrMsg(move(_threadErrorMsg->GetErrorMsg()));
    return TriBool::Error;
  }

  VectorDataValue vdv;
  VersionStamp stamp = vctProp[0]._tree->ApplyStamp(rangePos);
  LeafRecord *lrNew = lr->UpdateRecord(table->GetVectorIndex()[0]._tree, vdv,
                                       stamp, this, ActionType::DELETE, false);

  MVector<RawRecord *> &vctRec = page->GetRecords();
  vctRec[pagePos] = lrNew;

  int32_t commLen1, commLen2, tempLen1, tempLen2;
  lr->GetLength(tempLen1, commLen1);
  lrNew->GetLength(tempLen2, commLen2);
  page->UpdateDataLength(commLen2 - commLen1, tempLen2 - tempLen1);
  AddLeafRecord(lrNew);
  page->SetRecordUpdated();
  return TriBool::True;
}

StmtStatus StmtShowTables::SessionExec(Session *sess) {
  return StmtStatus::Finished;
}

StmtStatus StmtTrunTable::SessionExec(Session *sess) {
  return StmtStatus::Finished;
}

StmtStatus StmtTransaction::SessionExec(Session *sess) {
  return StmtStatus::Finished;
}

} // namespace storage