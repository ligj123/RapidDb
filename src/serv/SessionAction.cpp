#include "SessionAction.h"

#include "../expr/ExprAggr.h"
#include "../expr/ExprData.h"
#include "../expr/ExprDdl.h"
#include "../expr/ExprFunc.h"
#include "../expr/ExprLogic.h"
#include "../expr/ExprStatement.h"
#include "../manager/DatabaseManager.h"
#include "../manager/TableManager.h"
#include "../sql/Parser.h"
#include "../statement/DdlStatement.h"
#include "../statement/DeleteStatement.h"
#include "../statement/InsertStatement.h"
#include "../statement/StmtResult.h"
#include "../statement/TableSelectStatement.h"
#include "../statement/UpdateStatement.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"
#include "SessionPool.h"

#include <filesystem>

namespace fs = std::filesystem;
namespace storage {
TaskStatus SessionRecordAction::Exec(SessionGroup &sGroup) {
  _stmt->AddLeafRecords(_vctLr);
  return TaskStatus::FINISHED;
}

TaskStatus SessionErrMsgAction::Exec(SessionGroup &sGroup) {
  _stmt->GetStmtResult()->_vctError.push_back(move(_errMsg));
  return TaskStatus::FINISHED;
}

TaskStatus SessionRangeEndAction::Exec(SessionGroup &sGroup) {
  _stmt->SetFinished(true);
  return TaskStatus::FINISHED;
}

TaskStatus SessionCreateAction::Exec(SessionGroup &sGroup) {
  Session *session = new Session(_sessionId);
  sGroup._mapSession.emplace(_sessionId, session);
  _result->_sessionId = _sessionId;
  _result->SetResultStatus(ResultStatus::FINISHED);

  return TaskStatus::FINISHED;
}

TaskStatus SessionCloseAction::Exec(SessionGroup &sGroup) {
  auto iter = sGroup._mapSession.find(_sessionId);
  if (iter != sGroup._mapSession.end()) {
    Session *sess = iter->second;
    sess->_bObsolete = true;
    if (sess->_currStatement != nullptr) {
      sess->_currStatement->SetStmtFailed(true);
    }
    for (Statement *stmt : sess->_lstWaittingStmt) {
      stmt->SetStmtFailed(true);
    }

    sGroup._obsoleteSession.push_back(sess);
    sGroup._mapSession.erase(iter);
  } else {
    _result->_vctError.push_back("Failed to find session " +
                                 ToMString(_sessionId));
  }

  _result->_sessionId = _sessionId;
  _result->SetResultStatus(ResultStatus::FINISHED);

  return TaskStatus::FINISHED;
}

TaskStatus SessionStatementAction::Exec(SessionGroup &sGroup) {
  // _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  // return TaskStatus::FINISHED;

  _stmtResult->_sessionId = _sessionId;
  _stmtResult->_stmtId = _stmtId;
  auto iter = sGroup._mapSession.find(_sessionId);
  if (iter == sGroup._mapSession.end()) {
    _stmtResult->_vctError.push_back("Failed to find session " +
                                     ToMString(_sessionId));
    _stmtResult->SetResultStatus(ResultStatus::FINISHED);
    return TaskStatus::FINISHED;
  }

  Session *session = iter->second;
  ExprStatement *exprStmt = nullptr;
  uint64_t exprId = (static_cast<uint64_t>(_sessionId) << 32) + _exprId;
  auto itExpr = sGroup._mapIdExprStatement.find(exprId);

  if (itExpr == sGroup._mapIdExprStatement.end()) {
    MString dbSql =
        (session->_currDb == nullptr ? "" : session->_currDb->GetDbName());
    dbSql += _sql;
    auto itSql = sGroup._mapSqlExprStatement.find(dbSql);
    if (itSql == sGroup._mapSqlExprStatement.end()) {
      ParserResult result;
      bool b = Parser::Parse(_sql, result);
      if (!b) {
        _stmtResult->_vctError.push_back(move(result.ErrorMsg()));
        _stmtResult->SetResultStatus(ResultStatus::FINISHED);
        return TaskStatus::FINISHED;
      }

      MVectorPtr<ExprStatement *> *vctPtr = result.GetStatements();
      assert(vctPtr->size() == 1);

      exprStmt = vctPtr->at(0);
      vctPtr->clear();
      if (!exprStmt->Preprocess(session->_currDb)) {
        _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
        _stmtResult->SetResultStatus(ResultStatus::FINISHED);
        return TaskStatus::FINISHED;
      }

      if (exprStmt->IsCacheExpr()) {
        sGroup._mapSqlExprStatement.emplace(dbSql, exprStmt);
        sGroup._mapIdExprStatement.emplace(exprId, exprStmt);
      }
    } else {
      exprStmt = itSql->second;
      sGroup._mapIdExprStatement.emplace(exprId, exprStmt);
    }
  } else {
    exprStmt = itExpr->second;
    // #ifndef DNDEBUG
    //     LOG_INFO << "test";
    //     MString dbSql =
    //         (session->_currDb == nullptr ? "" :
    //         session->_currDb->GetDbName()) + _sql;
    //     auto iter = sGroup._mapSqlExprStatement.find(dbSql);
    //     assert(iter != sGroup._mapSqlExprStatement.end() &&
    //            iter->second == exprStmt);
    // #endif
  }

  // _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  // return TaskStatus::FINISHED;

  exprStmt->_dtLastVisit = MilliSecTime();
  Statement *stmt = nullptr;
  switch (exprStmt->GetType()) {
  case ExprType::EXPR_INSERT:
    stmt = new InsertStatement(_stmtId, TXID_NULL,
                               dynamic_cast<ExprInsert *>(exprStmt),
                               move(_vctParas), _stmtResult);
    break;
  case ExprType::EXPR_UPDATE:
    stmt = new UpdateStatement(_stmtId, TXID_NULL,
                               dynamic_cast<ExprUpdate *>(exprStmt),
                               move(_vctParas[0]), _stmtResult);
    break;
  case ExprType::EXPR_DELETE:
    stmt = new DeleteStatement(_stmtId, TXID_NULL,
                               dynamic_cast<ExprDelete *>(exprStmt),
                               move(_vctParas[0]), _stmtResult);
    break;
  case ExprType::EXPR_SELECT: {
    ExprStatement *exprDest =
        dynamic_cast<ExprSelect *>(exprStmt)->_exprDestSelect;
    if (exprDest->GetType() == ExprType::EXPR_TABLE_SELECT) {
      stmt = new TableSelectStatement(_stmtId, TXID_NULL,
                                      dynamic_cast<ExprTableSelect *>(exprDest),
                                      move(_vctParas[0]), _stmtResult);
    } else {
      LOG_FATAL << "Unsupport ExprType " << exprStmt->GetType();
      abort();
    }
    break;
  }
  case ExprType::EXPR_CREATE_DATABASE:
    stmt = new StmtCreateDatabase(_stmtId, TXID_NULL,
                                  dynamic_cast<ExprCreateDatabase *>(exprStmt),
                                  _stmtResult);
    break;
  case ExprType::EXPR_DROP_DATABASE:
    stmt = new StmtDropDatabase(_stmtId, TXID_NULL,
                                dynamic_cast<ExprDropDatabase *>(exprStmt),
                                _stmtResult);
    break;
  case ExprType::EXPR_SHOW_DATABASES:
    stmt = new StmtShowDatabases(_stmtId, TXID_NULL,
                                 dynamic_cast<ExprShowDatabases *>(exprStmt),
                                 _stmtResult);
    break;
  case ExprType::EXPR_USE_DATABASE:
    stmt = new StmtUseDatabase(_stmtId, TXID_NULL,
                               dynamic_cast<ExprUseDatabase *>(exprStmt),
                               _stmtResult);
    break;
  case ExprType::EXPR_CREATE_TABLE:
    stmt = new StmtCreateTable(_stmtId, TXID_NULL,
                               dynamic_cast<ExprCreateTable *>(exprStmt),
                               _stmtResult);
    break;
  case ExprType::EXPR_DROP_TABLE:
    stmt =
        new StmtDropTable(_stmtId, TXID_NULL,
                          dynamic_cast<ExprDropTable *>(exprStmt), _stmtResult);
    break;
  case ExprType::EXPR_SHOW_TABLES:
    stmt = new StmtShowTables(_stmtId, TXID_NULL,
                              dynamic_cast<ExprShowTables *>(exprStmt),
                              _stmtResult);
    break;
  case ExprType::EXPR_TRUN_TABLE:
    stmt =
        new StmtTrunTable(_stmtId, TXID_NULL,
                          dynamic_cast<ExprTrunTable *>(exprStmt), _stmtResult);
    break;
  case ExprType::EXPR_TRANSACTION:
    stmt = new StmtTransaction(_stmtId, TXID_NULL,
                               dynamic_cast<ExprTransaction *>(exprStmt),
                               _stmtResult);
    break;
  default:
    LOG_FATAL << "Unsupport ExprType " << exprStmt->GetType();
    abort();
    break;
  }

  // delete stmt;
  // _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  // return TaskStatus::FINISHED;

  session->_lstWaittingStmt.push_back(stmt);
  // if (!session->_bBusyQueue) {
  //   sGroup._lstBusySession.push_back(session);
  //   session->_bBusyQueue = true;
  // }

  return TaskStatus::FINISHED;
}

TaskStatus SessionUseDB::Exec(SessionGroup &sGroup) {
  auto iter = sGroup._mapSession.find(_sessionId);
  if (iter != sGroup._mapSession.end()) {
    Session *sess = iter->second;
    sess->_currDb = DatabaseManager::FindDb(_dbName);
    assert(sess->_currDb != nullptr);
  } else {
    _result->_vctError.push_back("Failed to find session " +
                                 ToMString(_sessionId));
  }

  _result->_sessionId = _sessionId;
  _result->SetResultStatus(ResultStatus::FINISHED);

  return TaskStatus::FINISHED;
}

TaskStatus SessionCleaner::Exec(SessionGroup &sGroup) {
  for (auto iter = sGroup._obsoleteSession.begin();
       iter != sGroup._obsoleteSession.end(); iter++) {
    if ((*iter)->IsEmpty()) {
      sGroup._obsoleteSession.erase(iter);
    } else {
      (*iter)->SetChechTime();
    }
  }

  for (auto iter = sGroup._mapSession.begin(); iter != sGroup._mapSession.end();
       iter++) {
    iter->second->SetChechTime();
  }

  for (auto iter = sGroup._mapIdExprStatement.begin();
       iter != sGroup._mapIdExprStatement.end();) {
    if (iter->second->_dtLastVisit < _dtStart &&
        _dtStart - iter->second->_dtLastVisit > 3600 * 1000ULL) {
      iter = sGroup._mapIdExprStatement.erase(iter);
    } else {
      iter++;
    }
  }

  for (auto iter = sGroup._mapSqlExprStatement.begin();
       iter != sGroup._mapSqlExprStatement.end();) {
    if (iter->second->_dtLastVisit < _dtStart &&
        _dtStart - iter->second->_dtLastVisit > 3600 * 1000ULL) {
      delete iter->second;
      iter = sGroup._mapSqlExprStatement.erase(iter);
    } else {
      iter->second->CheckObsoleteTable();
      iter++;
    }
  }

  size_t num = _cntFinished.fetch_add(1, memory_order_acq_rel);
  size_t gCnt = SessionPool::GetVctSessionGroup().size();
  if (num == gCnt - 1) {
    auto &vctTbl = TableManager::GetDiscardTable();
    for (auto iter = vctTbl.begin(); iter != vctTbl.end();) {
      if ((*iter)->GetLastCheckTime() < _dtStart) {
        PhysTable *tbl = *iter;
        bool bStoped = true;
        auto &vctTasks = tbl->GetTableTaskMgr()->GetVctIndexTasks();
        for (MVector<IndexTask *> &vctTask : vctTasks) {
          for (IndexTask *task : vctTask) {
            if (!task->IsRemovedPool()) {
              bStoped = false;
              break;
            }
          }

          if (!bStoped) {
            break;
          }
        }

        if (!bStoped) {
          tbl->GetDb()->SetCheckTime();
          tbl->SetCheckTime();
          continue;
        }

        if ((*iter)->GetTableStatus() == ResStatus::Droped) {
          fs::remove_all((*iter)->GetPath());
        }
        delete *iter;
        iter = vctTbl.erase(iter);
      } else {
        iter++;
      }
    }

    auto &vctDb = DatabaseManager::GetDiscardDb();
    for (auto iter = vctDb.begin(); iter != vctDb.end();) {
      if ((*iter)->GetLastCheckTime() < _dtStart) {
        if ((*iter)->GetResStatus() == ResStatus::Droped) {
          fs::remove_all((*iter)->GetDbPath());
        }

        delete *iter;
        iter = vctDb.erase(iter);
      } else {
        iter++;
      }
    }
  }

  return TaskStatus::FINISHED;
}

} // namespace storage