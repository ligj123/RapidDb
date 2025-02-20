#pragma once
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
class TableSelectStatement : public Statement {
public:
  TableSelectStatement(uint32_t id, TranID txid, ExprTableSelect *exprSelect,
                       VectorDataValue &&vctPara, StmtResult *result)
      : Statement(id, txid, exprSelect, result, move(vctPara)) {}
  ~TableSelectStatement() { assert(_lstStmtRec.size() == 0); }
  ExprType GetType() override { return ExprType::EXPR_TABLE_SELECT; }
  bool IsReadonly() override { return true; }

  StmtStatus SessionExec(Session *sess) override;

  TriBool HandleLeafRecord(LeafPage *page, int pagePos, int rangePos,
                           VectorLeafRecord *vctLeafRec = nullptr) override;

  ExprTableSelect *GetExprTableSelect() {
    return dynamic_cast<ExprTableSelect *>(_exprStmt);
  }

protected:
  // If The search index is secondary index, below variable to save the selected
  // primary key to primary index and used to pick the records.
  MList<StmtSecRecord *> _lstStmtRec;

  // The total number of updated LeafRecords, only valid when the search index
  // is secondary index
  uint32_t _totalRecNum{0};
};
} // namespace storage
