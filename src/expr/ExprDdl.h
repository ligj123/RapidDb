#pragma once
#include "../cache/Mallocator.h"
#include "../core/IndexType.h"
#include "BaseExpr.h"

using namespace std;
namespace storage {

class ExprCreateDatabase : public ExprStatement {
public:
  ExprCreateDatabase(MString name, bool ifNotExist)
      : _dbName(name), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_CREATE_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
};

class ExprDropDatabase : public ExprStatement {
public:
  ExprDropDatabase(MString name, bool ifNotExist)
      : _dbName(name), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_DROP_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
};

class ExprShowDatabases : public ExprStatement {
public:
  ExprType GetType() { return ExprType::EXPR_SHOW_DATABASES; }
};

class ExprUseDatabase : public ExprStatement {
public:
  ExprUseDatabase(MString name) : _dbName(name) {}
  ExprType GetType() { return ExprType::EXPR_USE_DATABASE; }

public:
  MString _dbName;
};

class ExprColumnInfo : public BaseExpr {
public:
  ExprType GetType() { return ExprType::EXPR_COLUMN_INFO; }

public:
  MString _colName;
  DataType _dataType;
  int _length;
  int _incStart;
  int _incStep;
  bool _nullable;
  unique_ptr<IDataValue> _defaultVal;
  bool _autoInc;
  IndexType _indexType;
  MString _comment;
};

class ExprConstraint : public BaseExpr {
public:
  ExprType GetType() { return ExprType::EXPR_CONSTRAINT; }

public:
  MString _idxName;
  IndexType _idxType;
  // In this version here does only support to include entire columns, and the
  // total length of columns can not exceed the index max length defined in
  // config. In following version, maybe to support more complex content.
  MVector<MString> _vctCol;
};

class ExprCreateTable : public ExprStatement {
public:
  ExprCreateTable(MString &tname, bool ifNotExist,
                  MVector<ExprColumnInfo> &vctCol)
      : _tName(move(tname)), _ifNotExist(ifNotExist), _vctCol(vctCol) {}
  ExprType GetType() { return ExprType::EXPR_CREATE_TABLE; }

public:
  MString _tName;
  bool _ifNotExist;
  MVector<ExprColumnInfo *> _vctCol;
  MVector<ExprConstaint *> _vctConst;
};

class ExprDropTable : public ExprStatement {
public:
  ExprDropTable(MString &tname, bool ifNotExist)
      : _tName(move(tname)), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_DROP_TABLE; }

public:
  MString _tName;
  bool _ifNotExist;
};

class ExprShowTable : public ExprStatement {
public:
  ExprShowTable(MString dbName) : _dbName(move(dbName)) {}
  ExprType GetType() { return ExprType::EXPR_SHOW_TABLE; }

public:
  MString _dbName;
};

class ExprTrunTable : public ExprStatement {
public:
  ExprTrunTable(MString dbName) : _dbName(move(dbName)) {}
  ExprType GetType() { return ExprType::EXPR_TRUN_TABLE; }

public:
  MString _dbName;
};

enum class TranType { BEGIN, COMMIT, ROLLBACK };
class ExprTransaction : public ExprStatement {
public:
  TranType _tranType;
};

enum class AlterAction { ADD, CHANGE, ALTER, MODIFY, DROP, RENAME };
class ExprAlterTable : public ExprStatement {
public:
public:
  AlterAction _action;
};
} // namespace storage