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
#include "SessionPool.h"

namespace storage {
TaskStatus SessionRecordAction::Exec() {
  assert(_lr->_recLock != nullptr);
  _stmt->AddLeafRecord(_lr);
  return TaskStatus::FINISHED;
}

TaskStatus SessionErrMsgAction::Exec() {
  _stmt->GetStmtResult()->_vctError.push_back(move(_errMsg));
  _stmt->SetStmtFailed(true);
  return TaskStatus::FINISHED;
}

TaskStatus SessionCreateAction::Exec() {
  MVector<SessionGroup> vctGroup = SessionPool::GetSessionGroup();
  uint64_t idx = _sessionId % vctGroup.size();
  SessionGroup &group = vctGroup[idx];
  Session *session = new Session(idx, _sessionId);
  group._mapSession.emplace(_sessionId, session);
  _result->_sessionId = _sessionId;
  _result->_status.store(ResultStatus::FINISHED, memory_order_release);

  return TaskStatus::FINISHED;
}

TaskStatus SessionCloseAction::Exec() {
  MVector<SessionGroup> vctGroup = SessionPool::GetSessionGroup();
  uint64_t idx = _sessionId % vctGroup.size();
  SessionGroup &group = vctGroup[idx];
  group._mapSession.erase(_sessionId);
  _result->_sessionId = _sessionId;
  _result->_status.store(ResultStatus::FINISHED, memory_order_release);

  return TaskStatus::FINISHED;
}

TaskStatus SessionStatementAction::Exec() {
  MVector<SessionGroup> vctGroup = SessionPool::GetSessionGroup();
  uint64_t idx = _sessionId % vctGroup.size();
  SessionGroup &group = vctGroup[idx];

  auto iter = group._mapSession.find(_sessionId);
  if (iter == group._mapSession.end()) {
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
      _stmtResult->_status.stor(ResultStatus::FINISHED, memory_order_release);
      return TaskStatus::FINISHED;
    }

    MVectorPtr<ExprStatement *> *vctPtr = result.GetStatements();
    assert(vctPtr->size() == 1);

    exprStmt = vctPtr[0];
    vctPtr->clear();
    // exprStmt->Preprocess(session);

    session->_mapSqlExprStatement.emplace(_sql, exprStmt);
    session->_mapIdExprStatement.emplace(_exprId, exprStmt);
  } else {
    exprStmt = itExpr->second;
  }

  Statement *stmt = nullptr;
  switch (exprStmt->GetType()) {
  case ExprType::EXPR_INSERT:
    stmt = new InsertStatement(_stmtId, TXID_NULL, exprStmt, _vctParas,
                               _stmtResult);
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