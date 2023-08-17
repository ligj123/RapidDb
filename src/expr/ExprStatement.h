#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../table/Table.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprAggr.h"
#include "ExprData.h"
#include "ExprLogic.h"
#include <unordered_set>

using namespace std;
namespace storage {
template <ExprType ET> class ExprCondition : public BaseExpr {
public:
  ExprCondition() {}
  ~ExprCondition() { delete _exprLogic; }
  ExprType GetDataType() const { return ET; }

  TriBool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_exprLogic == nullptr)
      return TriBool::True;
    return _exprLogic->Calc(vdPara, vdRow);
  }

public:
  ExprLogic *_exprLogic{nullptr};
};

typedef ExprCondition<ExprType::EXPR_ON> ExprOn;
typedef ExprCondition<ExprType::EXPR_HAVING> ExprHaving;

// To query physical table, point out which index will be used. Only one index
// can be selected.
struct UseIndex {
  // If the primary or secondary index can be used to query, copy the query
  // conditions to here. Only one index can be used. Only valid for physical
  // table select
  ExprLogic *_indexExpr;
  // which index used, The position of index that start from 0(primary key)
  int _indexPos;
};

class ExprWhere : public ExprCondition<ExprType::EXPR_WHERE> {
public:
  // This variable will be set when preprocess
  UseIndex *_useIndex{nullptr};
}

struct ExprGroupItem {
  MString _colName; // The column name, from sql statement
  int _pos;         // The position of column in result set
};

class ExprGroupBy : public BaseExpr {
public:
  ExprGroupBy(vector<ExprGroupItem> vct) { _vctItem.swap(vct); }
  ExprType GetType() { return ExprType::EXPR_GROUP_BY; }

public:
  vector<ExprGroupItem> _vctItem;
};

struct ExprOrderTerm {
  MString _colName;
  bool _direct; // True: ASC; False: DESC
  int _pos;
};

class ExprOrderBy : public BaseExpr {
public:
  ExprOrderBy(vector<ExprOrderTerm> &vct) { _vctItem.swap(vct); }
  ExprType GetType() { return ExprType::EXPR_ORDER_BY; }

public:
  vector<ExprOrderTerm> _vctItem;
};

class ExprTable : public BaseExpr {
public:
  ExprTable(string &dbName, string &name, string &alias)
      : _dbName(move(dbName)), _tName(move(name)), _tAlias(alias) {
    if (_tAlias.size() == 0)
      _tAlias = _tName;
  }
  ~ExprTable() {}
  ExprType GetType() { return ExprType::EXPR_TABLE; }

public:
  string _dbName;
  string _tName;
  string _tAlias;
};

enum class JoinType { INNER_JOIN, LEFT_JOIN, RIGHT_JOIN, OUTTER_JOIN };

class ExprJoinTable : public ExprTable {
public:
  ExprJoinTable(JoinType joinType, string &dbName, string &name, string &alias)
      : ExprTable(dbName, name, alias), _joinType(joinType) {}
  ExprType GetType() { return ExprType::EXPR_JOIN_TABLE; }

public:
  JoinType _joinType;
};

// Base class for all statement
class ExprStatement : public BaseExpr {
public:
  ExprStatement() {}
  ~ExprStatement() {}
  bool Preprocess() = 0;

public:
  int _paramNum; // The numbe of parameters in this statement
};

enum class LockType { NO_LOCK, SHARE_LOCK, WRITE_LOCK };

// This select class is only for parse, it will convert into a series of
// statement in preprocess.
class ExprSelect : public ExprStatement {
public:
  ExprSelect() {}
  ~ExprSelect() {
    for (ExprResColumn *rc : _vctCol) {
      delete rc;
    }
    for (ExprTable *t : _vctTable) {
      delete t;
    }
    delete _exprWhere;
    delete _exprGroupBy;
    delete _exprHaving;
    delete _exprOrderBy
  }
  ExprType GetType() { return ExprType::EXPR_SELECT; }
  bool Preprocess() {
    // TO DO
  }

public:
  // Remove repeated rows or not
  bool _bDistinct{false};
  // The selected columns
  MVector<ExprResColumn *> _vctCol;
  // The source tables, first table is ExprTable, the following tables are
  // ExprJoinTable if have.
  MVector<ExprTable *> _vctTable;
  // Where condition
  ExprWhere *_exprWhere{nullptr};

  ExprGroupBy *_exprGroupBy{nullptr};
  ExprHaving *_exprHaving{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};

  // offset for return rows, default 0. Only valid for top select result.
  int _offset{-1};
  // The max rows to return, -1 means return all. Only valid for top select
  // result.
  int _rowCount{-1};

  LockType _lockType{LockType::NO_LOCK};
};

class ExprInsert : public ExprStatement {
public:
  ExprInsert() {}

  ~ExprInsert() {
    delete _exprTable;
    delete _exprSelect;
    for (ExprColumn *col : _vctCol)
      delete col;
    for (ExprData *data _vctData)
      delete data;

    if (_physTable != nullptr)
      _physTable->DecRef();
  }

  ExprType GetType() { return ExprType::EXPR_INSERT; }
  bool Preprocess() {
    // TO DO
  }

public:
  // The destion table
  ExprTable *_exprTable{nullptr};
  // The columns that assign values; if empty, it will be filled with all
  // table's columns
  MVector<ExprColumn *> _vctCol;
  // The values that will be inserted.
  MVector<ExprData *> _vctData;
  // The source data that selected from other table and will be inserted into
  // this table
  ExprSelect *_exprSelect{nullptr};
  // True, update if the primary key has exist
  bool _bUpsert;
  // The physical table will insert into. Filled when preprocess
  PhysTable *_physTable{nullptr};
};

class ExprUpdate : public ExprStatement {
public:
  ExprUpdate() {}

  ~ExprUpdate() {
    if (_physTable != nullptr) {
      _physTable->DecRef();
    }
    for (ExprColumn *col : _vctCol) {
      delete col;
    }

    delete _exprTable;
    delete _where;
    delete _exprOrderBy;
  }

  ExprType GetType() { return ExprType::EXPR_UPDATE; }
  bool Preprocess() {
    // TO DO
  }

public:
  // The destion table information
  ExprTable *_exprTable;
  // The update columns and their values, have saved in ExprColumn
  MVector<ExprColumn *> _vctCol;
  // Where condition
  ExprWhere *_exprWhere;

  ExprOrderBy *_exprOrderBy;
  // The number of rows to delete from top
  int _rowCount{-1};
  // The physical table will update. Filled when preprocess
  PhysTable *_physTable{nullptr};
};

class ExprDelete : public ExprStatement {
public:
  ExprDelete() {}
  ~ExprDelete() {
    if (_physTable != nullptr) {
      _physTable->DecRef();
    }

    delete _where;
    delete _exprOrderBy;
  }

  ExprType GetType() { return ExprType::EXPR_DELETE; }
  bool Preprocess() {
    // TO DO
  }

public:
  // Where condition
  ExprWhere *_where;
  ExprOrderBy *_exprOrderBy;
  // The number of rows to delete from top
  int _rowCount{-1};
  // The physical table will update. Filled when preprocess
  PhysTable *_physTable{nullptr};
};
} // namespace storage
