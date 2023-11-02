#pragma once
#include "../cache/Mallocator.h"
#include "../core/IndexType.h"
#include "BaseExpr.h"
#include "ExprData.h"
#include "ExprLogic.h"

using namespace std;
namespace storage {
class Session;

class ExprCreateDatabase : public ExprStatement {
public:
  ExprCreateDatabase(MString *dbName, bool ifNotExist)
      : _dbName(dbName), _ifNotExist(ifNotExist) {}
  ~ExprCreateDatabase() { delete _dbName; }
  ExprType GetType() override { return ExprType::EXPR_CREATE_DATABASE; }
  bool Preprocess(Session *session = nullptr) override { return true; }

public:
  MString *_dbName;
  bool _ifNotExist;
};

class ExprDropDatabase : public ExprStatement {
public:
  ExprDropDatabase(MString *dbName, bool ifExist)
      : _dbName(dbName), _ifExist(ifExist) {}
  ~ExprDropDatabase() { delete _dbName; }
  ExprType GetType() override { return ExprType::EXPR_DROP_DATABASE; }
  bool Preprocess(Session *session = nullptr) override { return true; }

public:
  MString *_dbName;
  bool _ifExist;
};

class ExprShowDatabases : public ExprStatement {
public:
  ExprType GetType() override { return ExprType::EXPR_SHOW_DATABASES; }
  bool Preprocess(Session *session) override { return true; }
};

class ExprUseDatabase : public ExprStatement {
public:
  ExprUseDatabase(MString *dbName) : _dbName(dbName) {}
  ~ExprUseDatabase() { delete _dbName; }
  ExprType GetType() override { return ExprType::EXPR_USE_DATABASE; }
  bool Preprocess(Session *session = nullptr) override { return true; }

public:
  MString *_dbName;
};

class ExprDataType : public BaseExpr {
public:
  ExprDataType(DataType dataType, int32_t maxLen)
      : _dataType(dataType), _maxLen(maxLen) {}
  ExprDataType(DataType dataType) : _dataType(dataType), _maxLen(-1) {}
  ExprType GetType() override { return ExprType::EXPR_DATA_TYPE; }

public:
  DataType _dataType;
  int32_t _maxLen;
};

class ExprCreateTableItem : public BaseExpr {};

struct AutoIncrement {
  bool _autoInc;
  int64_t _initVal;
  int64_t _incStep;
};

class ExprColumnItem : public ExprCreateTableItem {
public:
  ~ExprColumnItem() {
    delete _colName;
    delete _comment;
    if (_defaultVal != nullptr)
      _defaultVal->Free();
  }
  ExprType GetType() override { return ExprType::EXPR_COLUMN_INFO; }

public:
  MString *_colName{nullptr};
  DataType _dataType{DataType::UNKNOWN};
  int32_t _maxLength{-1};
  bool _nullable{false};
  IDataValue *_defaultVal{nullptr};
  bool _autoInc{false};
  int64_t _initVal{-1};
  int64_t _incStep{-1};
  IndexType _indexType{IndexType::UNKNOWN};
  MString *_comment{nullptr};
};

class ExprTableIndex : public ExprCreateTableItem {
public:
  ExprTableIndex(MString *idxName, IndexType idxType,
                 MVectorPtr<MString *> *vctColName)
      : _idxName(idxName), _idxType(idxType), _vctColName(vctColName) {}
  ~ExprTableIndex() {
    delete _idxName;
    delete _vctColName;
  }
  ExprType GetType() override { return ExprType::EXPR_CONSTRAINT; }

public:
  MString *_idxName;
  IndexType _idxType;
  // In this version here does only support to include entire columns, and the
  // total length of columns can not exceed the index max length defined in
  // config. In following version, maybe to support more complex content.
  MVectorPtr<MString *> *_vctColName;
};

class ExprCreateTable : public ExprStatement {
public:
  ExprCreateTable(ExprTable *table, bool ifNotExist,
                  MVectorPtr<ExprCreateTableItem *> *vctItem)
      : _table(table), _ifNotExist(ifNotExist), _vctItem(vctItem) {}
  ~ExprCreateTable() {
    delete _table;
    delete _vctItem;
  }
  ExprType GetType() override { return ExprType::EXPR_CREATE_TABLE; }

  bool Preprocess(Session *session = nullptr) override;

public:
  ExprTable *_table;
  bool _ifNotExist;
  MVectorPtr<ExprCreateTableItem *> *_vctItem;
  // Split from _vctElem when preprocess
  MVectorPtr<ExprColumnItem *> _vctColumn;
  // Split from _vctElem when preprocess
  MVectorPtr<ExprTableIndex *> _vctIndex;
};

class ExprDropTable : public ExprStatement {
public:
  ExprDropTable(ExprTable *table, bool ifExist)
      : _table(table), _ifExist(ifExist) {}
  ~ExprDropTable() { delete _table; }
  ExprType GetType() override { return ExprType::EXPR_DROP_TABLE; }
  bool Preprocess(Session *session = nullptr) override;

public:
  ExprTable *_table;
  bool _ifExist;
};

class ExprShowTables : public ExprStatement {
public:
  ExprShowTables(MString *dbName) : _dbName(dbName) {}
  ~ExprShowTables() { delete _dbName; }

public:
  ExprType GetType() override { return ExprType::EXPR_SHOW_TABLES; }
  bool Preprocess(Session *session = nullptr) override;

public:
  MString *_dbName;
};

class ExprTrunTable : public ExprStatement {
public:
  ExprTrunTable(ExprTable *table) : _table(table) {}
  ~ExprTrunTable() { delete _table; }
  ExprType GetType() override { return ExprType::EXPR_TRUN_TABLE; }
  bool Preprocess(Session *session = nullptr) override;

public:
  ExprTable *_table;
};

class ExprTransaction : public ExprStatement {
public:
  ExprTransaction(TranAction tranAction) : _tranAction(tranAction) {}
  ExprType GetType() override { return ExprType::EXPR_TRANSACTION; }
  bool Preprocess(Session *session = nullptr) override;

public:
  TranAction _tranAction;
};

// enum class AlterAction : int8_t {
//   ADD = 0,
//   CHANGE,
//   ALTER,
//   MODIFY,
//   DROP,
//   RENAME
// };
// class ExprAlterTable : public ExprStatement {
// public:
//   bool Preprocess()override {
//     // TO DO
//     return false;
//   }

// public:
//   AlterAction _action;
// };
} // namespace storage