#pragma once
#include "../expr/ExprStatement.h"
#include "Statement.h"

namespace storage {
class InsertStatement : public Statement {
public:
  InsertStatement(ExprInsert *exprInsert, Transaction *tran);
  ~InsertStatement() {}
  ActionType GetActionType() override { return ActionType::INSERT; }
  int ExecuteUpdate() override;
  future<int> ExecuteUpdateAsyn() override;

protected:
  // ExprInsert will be managed unified by a class, do not delete here
  ExprInsert *_exprInsert;
};
} // namespace storage
