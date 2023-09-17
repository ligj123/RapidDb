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
  ExprCreateDatabase(MString *dbName, bool ifNotExist)
      : _dbName(dbName), _ifNotExist(ifNotExist) {}
  ~ExprCreateDatabase() { delete _dbName; }
  ExprType GetType() { return ExprType::EXPR_CREATE_DATABASE; }

public:
  MString *_dbName;
  bool _ifNotExist;
};

class ExprDropDatabase : public ExprStatement {
public:
  ExprDropDatabase(MString *dbName, bool ifExist)
      : _dbName(dbName), _ifExist(ifExist) {}
  ~ExprDropDatabase() { delete _dbName; }
  ExprType GetType() { return ExprType::EXPR_DROP_DATABASE; }

public:
  MString *_dbName;
  bool _ifExist;
};

class ExprShowDatabases : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_SHOW_DATABASES; }
};

class ExprUseDatabase : public ExprStatement {
public:
  ExprUseDatabase(MString *dbName) : _dbName(dbName) {}
  ~ExprUseDatabase() { delete _dbName; }
  ExprType GetType() { return ExprType::EXPR_USE_DATABASE; }

public:
  MString *_dbName;
};

class ExprDataType : public base {
public:
  ExprDataType(DataType dataType, int32_t maxLen)
      : _dataType(dataType), _maxLen(maxLen) {}
  ExprDataType(DataType dataType) : _dataType(dataType), _maxLen(-1) {}

public:
  DataType _dataType;
  int32_t _maxLen;
};

class ExprCreateTableItem : public BaseExpr {};

class ExprColumnItem : public ExprCreateTableItem {
public:
  ~ExprColumnItem() {
    delete _colName;
    delete _comment;
    delete _defaultVal;
  }
  ExprType GetType() { return ExprType::EXPR_COLUMN_INFO; }

public:
  MString *_colName{nullptr};
  DataType _dataType{DataType::UNKNOWN};
  int32_t _maxLength{-1};
  bool _nullable{false};
  IDataValue *_defaultVal{nullptr};
  bool _autoInc{false};
  IndexType _indexType{IndexType::UNKNOWN};
  MString *_comment{nullptr};
};

class ExprTableConstraint : public ExprCreateTableItem {
public:
  ExprTableConstraint(MString *idxName, IndexType idxType,
                      MVector<MString *> *vctColName)
      : _idxName(idxName), _idxType(idxType), _vctColName(vctColName) {}
  ~ExprConstraint() {
    delete _idxName;
    delete _vctColName;
  }
  ExprType GetType() { return ExprType::EXPR_CONSTRAINT; }

public:
  MString *_idxName;
  IndexType _idxType;
  // In this version here does only support to include entire columns, and the
  // total length of columns can not exceed the index max length defined in
  // config. In following version, maybe to support more complex content.
  MVector<MString *> *_vctColName;
};

class ExprCreateTable : public ExprStatement {
public:
  ExprCreateTable(ExprTable *tName, bool ifNotExist,
                  MVectorPtr<ExprTableItem *> *vctItem)
      : _tName(tName), _ifNotExist(ifNotExist), _vctItem(vctItem) {}
  ~ExprCreateTable() {
    delete _tName;
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
  MVectorPtr<ExprTableItem *> *_vctItem;
  // Split from _vctElem when preprocess
  MVectorPtr<ExprColumnItem *> *_vctColumn{nullptr};
  // Split from _vctElem when preprocess
  MVectorPtr<ExprTableConstraint *> *_vctConstraint{nullptr};
};

class ExprDropTable : public ExprStatement {
public:
  ExprDropTable(ExprTable *tName, bool ifExist)
      : _tName(tName), _ifExist(ifExist) {}
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
  ExprShowTables(MString *dbName) : _dbName(dbName) {}
  ~ExprShowTables() { delete _dbName; }

public:
  ExprType GetType() { return ExprType::EXPR_SHOW_TABLES; }
  bool Preprocess() {
    // TO DO
  }

public:
  MString *_dbName;
};

class ExprTrunTable : public ExprStatement {
public:
  ExprTrunTable(ExprTable *tableName) : _tableName(tableName) {}
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