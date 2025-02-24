#pragma once
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
class UpdateStatement : public Statement {
public:
  UpdateStatement(uint32_t id, TranID txid, ExprUpdate *expr,
                  VectorDataValue &&vctPara, StmtResult *result)
      : Statement(id, txid, expr, result, move(vctPara)) {}
  ~UpdateStatement() { assert(_lstStmtRec.size() == 0); }
  ExprType GetType() override { return ExprType::EXPR_UPDATE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;
  TriBool HandleLeafRecord(LeafPage *page, int pagePos, int rangePos,
                           VectorLeafRecord *vctLeafRec = nullptr) override;

  ExprUpdate *GetExprUpdate() { return dynamic_cast<ExprUpdate *>(_exprStmt); }
};
} // namespace storage
