#pragma once
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
class DeleteStatement : public Statement {
public:
  DeleteStatement(uint32_t id, TranID txid, ExprDelete *exprDelete,
                  VectorDataValue &&vctPara, StmtResult *result)
      : Statement(id, txid, exprDelete, result, move(vctPara)) {}
  ~DeleteStatement() {}
  ExprType GetType() override { return ExprType::EXPR_DELETE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;
  bool PrimaryKeyExec() override;
  bool SecondaryKeyExec() override;
  StmtStatus CheckStatus() override;
  void CollectLogRecords(TreeSetRecord &setRec) override;
  int CalcIndexRanges(IndexTree *idxTree) override;

  void Commit() override;
  void Rollback() override;
  ExprDelete *GetExprDelete() { return dynamic_cast<ExprDelete *>(_exprStmt); }

protected:
  // If The search index is secondary index, below variable to save the selected
  // primary key to primary index and used to pick the records.
  MList<StmtSecRecord *> _lstStmtRec;

  // The total number of updated LeafRecords, only valid when the search index
  // is secondary index
  uint32_t _totalRecNum{0};
};
} // namespace storage
