#include "Statement.h"

namespace storage {
class StmtCreateDatabase : public Statement {
public:
  StmtCreateDatabase(uint32_t id, Transaction *tran, ExprStatement *exprStmt)
      : Statement(id, tran, exprStmt, {}) {
    assert(exprStmt->GetType() == ExprType::EXPR_CREATE_DATABASE);
  }

  void Exec() override {}

  void WriteLog() override {}

  void Commit() override {}
};
} // namespace storage