#pragma once
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
// Normal insert, insert from select will implement in its brother class
class InsertStatement : public Statement {
public:
  InsertStatement(uint32_t id, Transaction *tran, ExprInsert *exprInsert,
                  VectorRow &&vctPara)
      : Statement(id, tran), _exprInsert(exprInsert), _vctPara(move(vctPara)) {}
  ~InsertStatement() {}
  ExprType GetActionType() override { return ExprType::EXPR_INSERT; }
  bool Exec() override;
  bool WriteLog() override;
  bool Commit() override;
  bool Abort() override;

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // All LeafRecord that are waiting to insert into or delete index tree,
  // include primary index and secondary index.
  MVector<LeafRecord *> _vctWaitRecord;
  // The LeafRecord that have inserted into or deleted from index tree.
  MVector<LeafRecord *> _vctFinshRecord;
  // The records' count that have insert into index tree.
  uint32_t _recFinished{0};
  VectorRow _vctPara;
};
} // namespace storage
