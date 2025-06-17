#pragma once
#include "../expr/ExprDdl.h"
#include "../expr/ExprStatement.h"
#include "../table/Table.h"
#include "Statement.h"

namespace storage {
class StmtCreateDatabase : public Statement {
public:
  StmtCreateDatabase(uint32_t id, TranID txid, ExprCreateDatabase *exprStmt,
                     StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_CREATE_DATABASE);
  }
  ~StmtCreateDatabase() {
    if (_instRecord != nullptr) {
      delete _instRecord;
    }

    if (_db != nullptr) {
      delete _db;
    }
  }
  ExprType GetType() override { return ExprType::EXPR_CREATE_DATABASE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;
  ExprCreateDatabase *GetExprStatement() {
    return dynamic_cast<ExprCreateDatabase *>(_exprStmt);
  }

  bool IsSoleTran() override { return true; }
  Database *GetDb() { return _db; }

protected:
  // To save the paras after handle
  StmtInsertRecord *_instRecord;
  // The database to create;
  Database *_db;
};

class StmtDropDatabase : public Statement {
public:
  StmtDropDatabase(uint32_t id, TranID txid, ExprDropDatabase *exprStmt,
                   StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_DROP_DATABASE);
  }

  ExprType GetType() override { return ExprType::EXPR_DROP_DATABASE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;
  ExprDropDatabase *GetExprStatement() {
    return dynamic_cast<ExprDropDatabase *>(_exprStmt);
  }

  bool IsSoleTran() override { return true; }

  TriBool HandleLeafRecord(LeafPage *page, int pagePos, int rangePos,
                           VectorLeafRecord *vctLeafRec = nullptr);

protected:
  bool _bDropingTable{true}; // If the database has tables and need to drop them
};

class StmtShowDatabases : public Statement {
public:
  StmtShowDatabases(uint32_t id, TranID txid, ExprShowDatabases *exprStmt,
                    StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_SHOW_DATABASES);
  }

  ExprType GetType() override { return ExprType::EXPR_SHOW_DATABASES; }
  bool IsReadonly() override { return true; }

  StmtStatus SessionExec(Session *sess) override;
  ExprShowDatabases *GetExprStatement() {
    return dynamic_cast<ExprShowDatabases *>(_exprStmt);
  }
};

class StmtUseDatabase : public Statement {
public:
  StmtUseDatabase(uint32_t id, TranID txid, ExprUseDatabase *exprStmt,
                  StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_USE_DATABASE);
  }

  ExprType GetType() override { return ExprType::EXPR_USE_DATABASE; }
  bool IsReadonly() override { return true; }

  StmtStatus SessionExec(Session *sess) override;
  ExprUseDatabase *GetExprStatement() {
    return dynamic_cast<ExprUseDatabase *>(_exprStmt);
  }
};

class StmtCreateTable : public Statement {
public:
  StmtCreateTable(uint32_t id, TranID txid, ExprCreateTable *exprStmt,
                  StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_CREATE_TABLE);
  }

  ExprType GetType() override { return ExprType::EXPR_CREATE_TABLE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;
  ExprCreateTable *GetExprStatement() {
    return dynamic_cast<ExprCreateTable *>(_exprStmt);
  }

  bool IsSoleTran() override { return true; }

protected:
  // To save the paras after handle
  StmtInsertRecord *_instRecord;
};

class StmtDropTable : public Statement {
public:
  StmtDropTable(uint32_t id, TranID txid, ExprDropTable *exprStmt,
                StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_DROP_TABLE);
  }

  ExprType GetType() override { return ExprType::EXPR_DROP_TABLE; }
  bool IsReadonly() override { return false; }

  StmtStatus SessionExec(Session *sess) override;
  ExprDropTable *GetExprStatement() {
    return dynamic_cast<ExprDropTable *>(_exprStmt);
  }

  TriBool HandleLeafRecord(LeafPage *page, int pagePos, int rangePos,
                           VectorLeafRecord *vctLeafRec = nullptr);

  bool IsSoleTran() override { return true; }
};

class StmtShowTables : public Statement {
public:
  StmtShowTables(uint32_t id, TranID txid, ExprShowTables *exprStmt,
                 StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_SHOW_TABLES);
  }

  ExprType GetType() override { return ExprType::EXPR_SHOW_TABLES; }
  bool IsReadonly() override { return true; }

  StmtStatus SessionExec(Session *sess) override;
  ExprShowTables *GetExprStatement() {
    return dynamic_cast<ExprShowTables *>(_exprStmt);
  }
};

class StmtTrunTable : public Statement {
public:
  StmtTrunTable(uint32_t id, TranID txid, ExprTrunTable *exprStmt,
                StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_TRUN_TABLE);
  }

  ExprType GetType() override { return ExprType::EXPR_TRUN_TABLE; }
  bool IsReadonly() override { return true; }

  StmtStatus SessionExec(Session *sess) override;
  ExprTrunTable *GetExprStatement() {
    return dynamic_cast<ExprTrunTable *>(_exprStmt);
  }
};

class StmtTransaction : public Statement {
public:
  StmtTransaction(uint32_t id, TranID txid, ExprTransaction *exprStmt,
                  StmtResult *result)
      : Statement(id, txid, exprStmt, result) {
    assert(exprStmt->GetType() == ExprType::EXPR_TRANSACTION);
  }

  ExprType GetType() override { return ExprType::EXPR_TRANSACTION; }
  bool IsReadonly() override { return true; }

  StmtStatus SessionExec(Session *sess) override;
  ExprTransaction *GetExprStatement() {
    return dynamic_cast<ExprTransaction *>(_exprStmt);
  }
};
} // namespace storage