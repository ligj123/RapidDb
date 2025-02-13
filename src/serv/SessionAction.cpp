#include "SessionAction.h"

#include "../../src/expr/ExprAggr.h"
#include "../../src/expr/ExprData.h"
#include "../../src/expr/ExprDdl.h"
#include "../../src/expr/ExprFunc.h"
#include "../../src/expr/ExprLogic.h"
#include "../../src/expr/ExprStatement.h"
#include "../../src/sql/Parser.h"
#include "../statement/InsertStatement.h"
#include "../statement/StmtResult.h"
#include "../utils/Log.h"
#include "SessionPool.h"

namespace storage {
TaskStatus SessionRecordAction::Exec(SessionGroup &sGroup) {
  for (LeafRecord *lr : _vctLr) {
    assert(lr->GetLock() != nullptr);
    _stmt->AddLeafRecord(lr);
  }
  return TaskStatus::FINISHED;
}

TaskStatus SessionErrMsgAction::Exec(SessionGroup &sGroup) {
  _stmt->GetStmtResult()->_vctError.push_back(move(_errMsg));
  _stmt->SetStmtFailed(true);
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
  auto iter = sGroup._mapSession.find(_sessionId);
  if (iter == sGroup._mapSession.end()) {
    _stmtResult->_vctError.push_back("Failed to find session " +
                                     ToMString(_sessionId));
    return TaskStatus::FINISHED;
  }

  ExprStatement *exprStmt = nullptr;
  Session *session = iter->second;
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
  }

  Statement *stmt = nullptr;
  switch (exprStmt->GetType()) {
  case ExprType::EXPR_INSERT:
    stmt = new InsertStatement(_stmtId, TXID_NULL, (ExprInsert *)exprStmt,
                               move(_vctParas), _stmtResult);
    break;
  case ExprType::EXPR_UPDATE:
    break;
  case ExprType::EXPR_DELETE:
    break;
  case ExprType::EXPR_SELECT:
    break;
  default:
    LOG_FATAL << "Unsupport ExprType " << exprStmt->GetType();
    abort();
    break;
  }

  session->_lstWaittingStmt.push_back(stmt);

  return TaskStatus::FINISHED;
}
} // namespace storage