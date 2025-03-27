#pragma once

#include "../core/LeafRecord.h"
#include "../statement/StmtResult.h"

namespace storage {
struct StmtResult;
struct SessionGroup;

class SessionAction {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
  virtual ~SessionAction() {}

  /**
   * @brief Run this action, and return the status to know if it has finished.
   */
  virtual TaskStatus Exec(SessionGroup &sGroup) = 0;
};

/**
 * @brief When insert or update a record, it will generate LeafRecords and add
 * them into related LeafPages. This action will send the LeafRecords into
 * SessionTask and then save them into statement for following steps.
 */
class SessionRecordAction : public SessionAction {
public:
  SessionRecordAction(Statement *stmt, MVector<LeafRecord *> &&vctLr)
      : _stmt(stmt), _vctLr(move(vctLr)) {}
  TaskStatus Exec(SessionGroup &sGroup) override;

protected:
  Statement *_stmt;
  MVector<LeafRecord *> _vctLr;
};

/**
 * @brief After finished a scan for the IndexRange, send this action to
 * SessionPool to notify the statement has finished index scan
 */
class SessionRangeEndAction : public SessionAction {
public:
  SessionRangeEndAction(Statement *stmt, int32_t rangePos, int32_t recNum)
      : _stmt(stmt), _rangePos(rangePos), _recNum(recNum) {}
  TaskStatus Exec(SessionGroup &sGroup) override;

protected:
  Statement *_stmt;
  int32_t _rangePos;
  int32_t _recNum;
};

/**
 * @brief If meet error when executing statement, to use this action to send the
 * error message into SessionTask and then save it into statement.
 */
class SessionErrMsgAction : public SessionAction {
public:
  SessionErrMsgAction(Statement *stmt, MString &&errMsg)
      : _stmt(stmt), _errMsg(move(errMsg)) {}
  ~SessionErrMsgAction() {}
  TaskStatus Exec(SessionGroup &sGroup) override;

protected:
  Statement *_stmt;
  MString _errMsg;
};

class SessionCreateAction : public SessionAction {
public:
  SessionCreateAction(uint32_t sid, StmtResult *result)
      : _sessionId(sid), _result(result) {}

  TaskStatus Exec(SessionGroup &sGroup) override;

protected:
  uint32_t _sessionId;
  StmtResult *_result;
};

class SessionCloseAction : public SessionAction {
public:
  SessionCloseAction(uint32_t sid, StmtResult *result)
      : _sessionId(sid), _result(result) {}

  TaskStatus Exec(SessionGroup &sGroup) override;

protected:
  uint32_t _sessionId;
  StmtResult *_result;
};

class SessionStatementAction : public SessionAction {
public:
  SessionStatementAction(uint32_t sessionId, uint32_t stmtId, uint32_t exprId,
                         MString &&sql, VectorRow &&vctParas,
                         StmtResult *result)
      : _sessionId(sessionId), _stmtId(stmtId), _exprId(exprId),
        _sql(move(sql)), _vctParas(move(vctParas)), _stmtResult(result) {}

  TaskStatus Exec(SessionGroup &sGroup) override;
  void ResetStmtResult() { _stmtResult->Reset(); }

protected:
  uint32_t _sessionId;
  uint32_t _stmtId;
  uint32_t _exprId;
  MString _sql;
  VectorRow _vctParas;
  StmtResult *_stmtResult;
};

class SessionUseDB : public SessionAction {
public:
  SessionUseDB(uint32_t sid, StmtResult *result, const MString &dbName)
      : _sessionId(sid), _result(result), _dbName(dbName) {}

  TaskStatus Exec(SessionGroup &sGroup) override;

protected:
  uint32_t _sessionId;
  StmtResult *_result;
  MString _dbName;
};
} // namespace storage