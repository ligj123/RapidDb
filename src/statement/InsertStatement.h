#pragma once
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
enum class InsertStatus : uint8_t {
  INSERT_START = 0,
  PRIMARY_PAGE_LOAD,
  SECONDARY_PAGE_LOAD
};
// Normal insert, insert from select will implement in its brother class
class InsertStatement : public Statement {
public:
  InsertStatement(ExprInsert *exprInsert, Transaction *tran);
  ~InsertStatement() {}
  ActionType GetActionType() override { return ActionType::INSERT; }
  TaskStatus Execute() override;

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // The current record to insert
  int32_t _currRow = 0;
  // Which record is inserting IndexTree, the position in _vctRecord
  int32_t _currRec = -1;
  // If an IndexPage is not in memory, it will load this page from disk. In this
  // moment, current task will pause and use below variable to save the page to
  // restart this task.
  IndexPage *_indexPage = nullptr;
  // Status
  InsertStatus _status = InsertStatus::INSERT_START;
};
} // namespace storage
