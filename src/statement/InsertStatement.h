#pragma once
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../expr/ExprStatement.h"
#include "../utils/ThreadPool.h"
#include "Statement.h"

namespace storage {
// To save the the paras of inserted records after split.
struct InsertRecord {
  RawKey _priKey;
  VectorDataValue _vctParas;
};

// Normal insert, insert from select will implement in its brother class
class InsertStatement : public Statement {
public:
  InsertStatement(uint32_t id, TranID txid, ExprInsert *exprInsert,
                  VectorRow &&vctParas, StmtResult *result)
      : Statement(id, txid, result), _exprInsert(exprInsert),
        _vctParas(move(vctParas)) {}
  ~InsertStatement() {}
  ExprType GetActionType() override { return ExprType::EXPR_INSERT; }
  bool IsReadonly() override { return false; }

  bool SessionExec() override;
  bool PrimaryKeyExec() override;

  StmtStatus CheckStatus() override;
  void CollectLogRecords(MTreeSet<LeafRecord *> &setRec) override;

  void Commit() override;
  void Rollback() override;

protected:
  // ExprInsert will be unified managed by a class, do not delete here
  ExprInsert *_exprInsert;
  // To save multi rows of parameters loaded from client byte array
  VectorRow _vctParas;
  // To save the paras after split， Map<RangPos, InsertRecord>
  MHashMap<int, MVectorPtr<InsertRecord *>> _mapInsert;
};
} // namespace storage
