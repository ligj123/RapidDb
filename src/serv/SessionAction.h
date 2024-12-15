#pragma once

#include "../core/LeafRecord.h"

namespace storage {

class SessionAction : public ThreadAction {};

class SessionRecordAction : public SessionAction {
public:
  SessionRecordAction(LeafRecord *lr) : _lr(lr) {}
  TaskStatus Exec() override;

protected:
  LeafRecord *_lr;
};

class SessionErrMsgAction : public SessionAction {
public:
  SessionErrMsgAction(ErrorMsg *errMsg) : _errMsg(errMsg) {}
  ~SessionErrMsgAction() { delete _errMsg; }
  TaskStatus Exec() override;

protected:
  ErrorMsg *_errMsg;
};

} // namespace storage