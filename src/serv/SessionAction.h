#pragma once

#include "../core/LeafRecord.h"

namespace storage {

class SessionAction : public ThreadAction {};

class SessionRecordAction : public SessionAction {
public:
  SessionRecordAction(Statement *stmt, LeafRecord *lr) : _stmt(stmt), _lr(lr) {}
  TaskStatus Exec() override;

protected:
  Statement *_stmt;
  LeafRecord *_lr;
};

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
  SessionCreateAction(uint32_t sid) : _sessionId(sid) {}

  TaskStatus Exec() override;

protected:
  uint32_t _sessionId;
};

class SessionCloseAction : public SessionAction {
  SessionCloseAction(uint32_t sid) : _sessionId(sid) {}

  TaskStatus Exec() override;

protected:
  uint32_t _sessionId;
};
} // namespace storage