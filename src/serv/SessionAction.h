#pragma once

#include "../core/LeafRecord.h"

namespace storage {
class StmtResult;

class SessionAction : public ThreadAction {};

/**
 * @brief When insert or update a record, it will generate LeafRecords and add
 * them into related LeafPages. This action will send the LeafRecords into
 * SessionTask and then save them into statement for following steps.
 */
class SessionRecordAction : public SessionAction {
public:
  SessionRecordAction(Statement *stmt, LeafRecord *lr) : _stmt(stmt), _lr(lr) {}
  TaskStatus Exec() override;

protected:
  Statement *_stmt;
  LeafRecord *_lr;
};

/**
 * @brief If meet error when executing statement, to use this action to send the
 * error message into SessionTask and then save it into statement.
 */
class SessionErrMsgAction : public SessionAction {
public:
  SessionErrMsgAction(Statement *stmt, ErrorMsg *errMsg)
      : _stmt(stmt), _errMsg(errMsg) {}
  ~SessionErrMsgAction() { delete _errMsg; }
  TaskStatus Exec() override;

protected:
  Statement *_stmt;
  ErrorMsg *_errMsg;
};

class SessionCreateAction : public SessionAction {
public:
  SessionCreateAction(uint32_t sid, StmtResult *result)
      : _sessionId(sid), _result(result) {}

  TaskStatus Exec() override;

protected:
  uint32_t _sessionId;
  StmtResult *_result;
};

class SessionCloseAction : public SessionAction {
  SessionCloseAction(uint32_t sid, StmtResult *result)
      : _sessionId(sid), _result(result) {}

  TaskStatus Exec() override;

protected:
  uint32_t _sessionId;
  StmtResult *_result;
};

class SessionStatementAction : public SessionAction {
public:
  SessionStatementAction(uint32_t sessionId, uint32_t stmtId, uint32_t exprId,
                         MString &&sql, VectorRow &&vctParas,
                         StmtResult *result)
      : _sessionId(sessionId), _stmtId(stmtId), _exprId, _sql(move(sql)),
        _vctParas(move(vctParas)), _stmtResult(result) {}

  TaskStatus Exec() override;

protected:
  uint32_t _sessionId;
  uint32_t _stmtId;
  uint32_t _exprId;
  MString _sql;
  VectorRow _vctParas;
  StmtResult *_stmtResult;
}
} // namespace storage