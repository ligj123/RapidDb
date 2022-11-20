#pragma once
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
enum class InsertStatus : uint8_t {
  PRIMARY_START = 0,
  PRIMARY_PROCESS,
  SECONDARY_PROCESS
};
// Normal insert, insert from select will implement in its brother class
class InsertStatement : public Statement {
public:
  InsertStatement(ExprInsert *exprInsert, Transaction *tran);
  ~InsertStatement() {}
  ActionType GetActionType() override { return ActionType::INSERT; }
  int Execute() override;

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // all inserted leaf records, inlude primary key and secondary key
  MVector<LeafRecord *> _vctRec;
  // The current record to insert
  int32_t _currRow = 0;
  // Which record is inserting IndexTree, the position in _vctRec
  int32_t _currRec = 0;
  // If an IndexPage is not in memory, it will load this page from disk. In this
  // moment, current task will pause and use below variable to save the page to
  // restart this task.
  IndexPage *_indexPage;
  // Status
  InsertStatus _status = InsertStatus::PRIMARY_START;
};
} // namespace storage
