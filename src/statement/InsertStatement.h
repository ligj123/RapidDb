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
      : Statement(id, txid, result), _exprInsert(exprInsert),
        _vctParas(move(vctParas)) {}
  ~InsertStatement() {}
  ExprType GetType() override { return ExprType::EXPR_INSERT; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;

  StmtStatus CheckStatus() override;
  void
  CollectLogRecords(MTreeSet<LeafRecord *, LeafRecordCmp> &setRec) override;

  void Commit() override;
  void Rollback() override;
  ExprInsert *GetExprInsert() { return _exprInsert; }

protected:
  bool InitRecord();

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // To save multi rows of parameters loaded from client byte array
  VectorRow _vctParas;
  // To save the paras after handle
  MList<StmtInsertRecord *> _lstRecord;
  // The total number of inserted records
  uint32_t _recordCount{0};
};
} // namespace storage
