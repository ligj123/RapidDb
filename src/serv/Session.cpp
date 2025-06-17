#include "Session.h"

#include "../statement/Statement.h"
#include "SessionPool.h"

namespace storage {
Session::Session(uint32_t id) : _id(id), _transaction(this) {}

Session::~Session() {
  assert(_transaction.GetTranStatus() == TranStatus::FINISHED ||
         _transaction.GetTranStatus() == TranStatus::INIT);
  assert(_lstWaittingStmt.size() == 0 && _currStatement == nullptr);
}

void Session::Exec() {
  if (_currStatement == nullptr) {
    if (_lstWaittingStmt.size() == 0) {
      return;
    }

    TranStatus ts = _transaction.GetTranStatus();
    assert(ts != TranStatus::AUTO_TRAN);
    _currStatement = _lstWaittingStmt.front();
    _lstWaittingStmt.pop_front();

    if (_currStatement->IsSoleTran() && ts == TranStatus::IN_TRAN) {
      // This version does not support auto coomit when run DDL statement.
      // This design maybe is changed in future.
      _currStatement->FailWithUnfinishedTran();
      delete _currStatement;
      _currStatement = nullptr;
      return;
    }

    if (_currStatement->GetType() == ExprType::EXPR_TRANSACTION) {
      StmtStatus s = _currStatement->SessionExec(this);
      if (s == StmtStatus::Finished) {
        delete _currStatement;
        _currStatement = nullptr;
      }

      return;
    }

    if (ts == TranStatus::FINISHED || ts == TranStatus::INIT) {
      _transaction.StartTransaction(
          _currStatement->IsSoleTran() ? true : _bAutoCommit);
    }

    _currStatement->SetTxID(_transaction.GetTranID());
    _transaction.AddStatement(_currStatement);
  }

  StmtStatus s = _currStatement->SessionExec(this);
  if (s == StmtStatus::Finished) {
    _transaction.CloseTransaction();
    _currStatement = nullptr;
  } else if (s == StmtStatus::Executed) {
    assert(!_transaction.IsAutoCommit());
    _currStatement = nullptr;
  }
}

void Session::SetChechTime() {
  if (_currDb != nullptr) {
    if (_bObsolete) {
      _currDb = nullptr;
    } else if (_currDb->IsObsolete())
      _currDb = nullptr;
  }

  for (Statement *stmt : _lstWaittingStmt) {
    stmt->GetExprStatement()->CheckObsoleteTable();
  }

  MList<Statement *> &lst = _transaction.GetListStatement();
  for (Statement *stmt : lst) {
    stmt->GetExprStatement()->CheckObsoleteTable();
  }
}
} // namespace storage