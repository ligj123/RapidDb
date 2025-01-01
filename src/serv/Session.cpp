#include "Session.h"

#include "../statement/Statement.h"
#include "SessionPool.h"

namespace storage {
Session::Session(uint32_t id) : _id(id), _transaction(this) {}

Session::~Session() {
  assert(_transaction.GetTranStatus() == TranStatus::FINISHED);
  assert(_lstWaittingStmt.size() == 0 && _currStatement == nullptr);
  for (auto iter = _mapSqlExprStatement.begin();
       iter != _mapSqlExprStatement.end(); iter++) {
    delete iter->second;
  }
}

void Session::Exec() {}
} // namespace storage