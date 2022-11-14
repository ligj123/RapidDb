#pragma once
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
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
  // In this version all records in this statement will be insert in one task,
  // but in fututre maybe use small tasks and enery small task will finish one
  // LeafRecord insert.
  // bool _bSmallTask;
};
} // namespace storage
