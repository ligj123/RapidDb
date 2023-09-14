#pragma once
#include "../cache/Mallocator.h"
#include "../core/IndexType.h"
#include "BaseExpr.h"
#include "ExprData.h"
#include "ExprLogic.h"

using namespace std;
namespace storage {

class ExprCreateDatabase : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_CREATE_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
};

class ExprDropDatabase : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_DROP_DATABASE; }

public:
  MString _dbName;
  bool _ifExist;
};

class ExprShowDatabases : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_SHOW_DATABASES; }
};

class ExprUseDatabase : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_USE_DATABASE; }

public:
  MString _dbName;
};

class ExprTableElem : public BaseExpr {};

class ExprColumnInfo : public ExprTableElem {
public:
  ExprType GetType() { return ExprType::EXPR_COLUMN_INFO; }

public:
  MString _colName;
  DataType _dataType;
  int _incStart;
  int _incStep;
  bool _nullable;
  unique_ptr<IDataValue> _defaultVal;
  bool _autoInc;
  IndexType _indexType;
  MString _comment;
};

class ExprConstraint : public ExprTableElem {
public:
  ~ExprConstraint() { delete _vctCol; }
  ExprType GetType() { return ExprType::EXPR_CONSTRAINT; }

public:
  MString _idxName;
  IndexType _idxType;
  // In this version here does only support to include entire columns, and the
  // total length of columns can not exceed the index max length defined in
  // config. In following version, maybe to support more complex content.
  MVector<MString> *_vctCol;
};

class ExprCreateTable : public ExprStatement {
public:
  ~ExprCreateTable() {
    delete _tName;
    delete _vctElem;
    delete _vctColumn;
    delete _vctConstraint
  }
  ExprType GetType() { return ExprType::EXPR_CREATE_TABLE; }
  bool Preprocess() {
    // TO DO
  }

public:
  ExprTable *_tName;
  bool _ifNotExist;
  MVector<ExprTableElem *> *_vctElem{nullptr};
  // Split from _vctElem when preprocess
  MVector<ExprColumnInfo *> *_vctColumn{nullptr};
  // Split from _vctElem when preprocess
  MVector<ExprConstaint *> *_vctConstraint{nullptr};
};

class ExprDropTable : public ExprStatement {
public:
  ~ExprDropTable() { delete _tName; }
  ExprType GetType() { return ExprType::EXPR_DROP_TABLE; }
  bool Preprocess() {
    // TO DO
  }

public:
  ExprTable *_tName;
  bool _ifExist;
};

class ExprShowTables : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_SHOW_TABLES; }
  bool Preprocess() {
    // TO DO
  }

public:
  MString _dbName;
};

class ExprTrunTable : public ExprStatement {
public:
  ~ExprTrunTable() { delete _tableName; }
  ExprType GetType() { return ExprType::EXPR_TRUN_TABLE; }
  bool Preprocess() {
    // TO DO
  }

public:
  ExprTable *_tableName;
};

enum class TranType { BEGIN, COMMIT, ROLLBACK };
class ExprTransaction : public ExprStatement {
public:
  ExprTransaction(TranType tranType) : _tranType(tranType) {}
  ExprType GetType() { return ExprType::EXPR_TRANSACTION; }

public:
  TranType _tranType;
};

enum class AlterAction { ADD, CHANGE, ALTER, MODIFY, DROP, RENAME };
class ExprAlterTable : public ExprStatement {
public:
  bool Preprocess() {
    // TO DO
  }

public:
  AlterAction _action;
};
} // namespace storage