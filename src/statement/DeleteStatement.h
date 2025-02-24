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
  ~DeleteStatement() { assert(_lstStmtRec.size() == 0); }
  ExprType GetType() override { return ExprType::EXPR_DELETE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;

  TriBool HandleLeafRecord(LeafPage *page, int pagePos, int rangePos,
                           VectorLeafRecord *vctLeafRec = nullptr) override;

  ExprDelete *GetExprDelete() { return dynamic_cast<ExprDelete *>(_exprStmt); }
};
} // namespace storage
