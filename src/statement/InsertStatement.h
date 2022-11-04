#pragma once
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
class InsertStatement : public Statement {
public:
  InsertStatement(ExprInsert *exprInsert, Transaction *tran);
  ~InsertStatement() {}
  ActionType GetActionType() override { return ActionType::INSERT; }
  int ExecuteUpdate() override;

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // all inserted leaf records, inlude primary key and secondary key
  MVector<LeafRecord *> _vctRec;
};
} // namespace storage
