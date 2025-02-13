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
  InsertStatement(uint32_t id, TranID txid, ExprInsert *exprInsert,
                  VectorRow &&vctParas, StmtResult *result)
      : Statement(id, txid, exprInsert, result), _vctParas(move(vctParas)) {}
  ~InsertStatement() {}
  ExprType GetType() override { return ExprType::EXPR_INSERT; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;

  ExprInsert *GetExprInsert() { return dynamic_cast<ExprInsert *>(_exprStmt); }

protected:
  bool InitRecord();

protected:
  // To save multi rows of parameters loaded from client byte array
  VectorRow _vctParas;
  // To save the paras after handle
  MList<StmtInsertRecord *> _lstRecord;
  // The total number of inserted LeafRecords
  uint32_t _cntLeafRec{0};
  // The number of inserted records
  uint32_t _recordNum{0};
};
} // namespace storage
