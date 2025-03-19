#include "SessionAction.h"

#include "../expr/ExprAggr.h"
#include "../expr/ExprData.h"
#include "../expr/ExprDdl.h"
#include "../expr/ExprFunc.h"
#include "../expr/ExprLogic.h"
#include "../expr/ExprStatement.h"
#include "../manager/DatabaseManager.h"
#include "../sql/Parser.h"
#include "../statement/DeleteStatement.h"
#include "../statement/InsertStatement.h"
#include "../statement/StmtResult.h"
#include "../statement/TableSelectStatement.h"
#include "../statement/UpdateStatement.h"
#include "../utils/Log.h"
#include "SessionPool.h"

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
  _result->_status.store(ResultStatus::FINISHED, memory_order_release);

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
  _result->_status.store(ResultStatus::FINISHED, memory_order_release);

  return TaskStatus::FINISHED;
}

TaskStatus SessionStatementAction::Exec(SessionGroup &sGroup) {
  _stmtResult->_sessionId = _sessionId;
  _stmtResult->_stmtId = _stmtId;
  auto iter = sGroup._mapSession.find(_sessionId);
  if (iter == sGroup._mapSession.end()) {
    _stmtResult->_vctError.push_back("Failed to find session " +
                                     ToMString(_sessionId));
    return TaskStatus::FINISHED;
  }

  Session *session = iter->second;
  ExprStatement *exprStmt = nullptr;
  auto itExpr = session->_mapIdExprStatement.find(_exprId);
  if (itExpr == session->_mapIdExprStatement.end()) {
    ParserResult result;
    bool b = Parser::Parse(_sql, result);
    if (!b) {
      _stmtResult->_vctError.push_back(move(result.ErrorMsg()));
      _stmtResult->_status.store(ResultStatus::FINISHED, memory_order_release);
      return TaskStatus::FINISHED;
    }

    MVectorPtr<ExprStatement *> *vctPtr = result.GetStatements();
    assert(vctPtr->size() == 1);

    exprStmt = vctPtr->at(0);
    vctPtr->clear();
    if (!exprStmt->Preprocess(session->_currDb)) {
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      _stmtResult->_status.store(ResultStatus::FINISHED, memory_order_release);
      return TaskStatus::FINISHED;
    }

    session->_mapSqlExprStatement.emplace(_sql, exprStmt);
    session->_mapIdExprStatement.emplace(_exprId, exprStmt);
  } else {
    exprStmt = itExpr->second;
    assert(session->_mapSqlExprStatement.find(_sql) !=
           session->_mapSqlExprStatement.end());
  }

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
  default:
    LOG_FATAL << "Unsupport ExprType " << exprStmt->GetType();
    abort();
    break;
  }

  session->_lstWaittingStmt.push_back(stmt);
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
  _result->_status.store(ResultStatus::FINISHED, memory_order_release);

  return TaskStatus::FINISHED;
}
} // namespace storage