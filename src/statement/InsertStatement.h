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
  bool IsReadonly() override { return false; }
  bool Exec() override;
  void CollectRecords(MTreeSet<LeafRecord *> &setRec) override;
  void Commit() override;
  void Rollback() override;

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // To save multi rows of parameters loaded from client byte array
  VectorRow _vctParas;
  // All LeafRecord that are waiting to insert into or delete index tree,
  // include primary index and secondary index.
  MForward_list<LeafRecord *> _lstWaitRecord;
  // The LeafRecord that have inserted into or deleted from index tree.
  MForward_list<LeafRecord *> _lstFinshRecord;
};
} // namespace storage
