#include "Session.h"

#include "../statement/Statement.h"
#include "SessionPool.h"

namespace storage {
Session::Session(uint32_t id) : _id(id), _transaction(this) {}

Session::~Session() {
  assert(_transaction.GetTranStatus() == TranStatus::FINISHED);
  assert(_lstWaittingStmt.size() == 0 && _currStatement == nullptr);
}

bool Session::Exec() {
  if (_currStatement == nullptr) {
    assert(_lstWaittingStmt.size() != 0);

    _currStatement = _lstWaittingStmt.front();
    _lstWaittingStmt.pop_front();

    if (_currStatement->GetType() == ExprType::EXPR_TRANSACTION) {
      StmtStatus s = _currStatement->SessionExec(this);
      if (s == StmtStatus::Finished) {
        delete _currStatement;
        _currStatement = nullptr;
      }

      return _lstWaittingStmt.size() != 0;
    }

    TranStatus ts = _transaction.GetTranStatus();
    if (ts == TranStatus::FINISHED || ts == TranStatus::INIT) {
      _transaction.StartTransaction(_bAutoCommit);
    }

    _currStatement->SetTxID(_transaction.GetTranID());
    _transaction.AddStatement(_currStatement);
  }

  StmtStatus s = _currStatement->SessionExec(this);
  if (s == StmtStatus::Finished) {
    _transaction.CloseTransaction();
    _currStatement = nullptr;
    return _lstWaittingStmt.size() > 0;
  } else if (s == StmtStatus::Executed) {
    assert(!_transaction.IsAutoCommit());
    _currStatement = nullptr;
    return _lstWaittingStmt.size() > 0;
  }

  return true;
}
} // namespace storage