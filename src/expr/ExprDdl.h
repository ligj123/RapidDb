#pragma once
#include "BaseExpr.h"

using namespace std;
namespace storage {

class ExprCreateDatabase : public BaseExpr {
public:
  ExprCreateDatabase(MString name, bool ifNotExist)
      : _dbName(name), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_CREATE_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
};

class ExprDropDatabase : public BaseExpr {
public:
  ExprDropDatabase(MString name, bool ifNotExist)
      : _dbName(name), _ifNotExist(ifNotExist) {}
  ExprType GetType() { return ExprType::EXPR_DROP_DATABASE; }

public:
  MString _dbName;
  bool _ifNotExist;
};

class ExprShowDatabases : public BaseExpr {
public:
  ExprType GetType() { return ExprType::EXPR_SHOW_DATABASES; }
};

class ExprUseDatabase : public BaseExpr {
public:
  ExprUseDatabase(MString name) : _dbName(name) {}
  ExprType GetType() { return ExprType::EXPR_USE_DATABASE; }

public:
  MString _dbName;
};

struct ExprColumnInfo {
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

class ExprCreateTable : public BaseExpr {
public:
  ExprCreateTable(MString &tname, bool ifNotExist, MVector<ExprColumnInfo> &vctCol)
      : _tName(move(tname)), _ifNotExist(ifNotExist), _vctCol(vctCol) {}

public:
  MString _tName;
  bool _ifNotExist;
  MVector<ExprColumnInfo> _vctCol;
};

class ExprDropTable : public BaseExpr {
public:
  ExprDropTable(MString &tname, bool ifNotExist)
      : _tName(move(tname)), _ifNotExist(ifNotExist) {}

public:
  MString _tName;
  bool _ifNotExist;
};

class ExprShowTable : public BaseExpr {
public:
  ExprShowTable(MString dbName) : _dbName(move(dbName)) {}

public:
  MString _dbName;
};

enum class AlterAction { ADD, CHANGE, ALTER, MODIFY, DROP, RENAME };
class ExprAlterTable : public BaseExpr {
public:
public:
  AlterAction _action;
};

} // namespace storage