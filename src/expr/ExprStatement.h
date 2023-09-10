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
  ~ExprWhere() { delete _useIndex; }

public:
  // This variable will be set when preprocess
  UseIndex *_useIndex{nullptr};
}

class ExprGroupBy : public BaseExpr {
public:
  ExprGroupBy(MVector<MString> vctColName) { _vctItemName.swap(vctColName); }
  ~ExprGroupBy() { delete _exprHaving; }
  ExprType GetType() { return ExprType::EXPR_GROUP_BY; }

public:
  MVector<MString> _vctColName;
  MVector<int> _vctColPos;
  ExprHaving *_exprHaving{nullptr};
};

struct ExprOrderItem {
  MString _colName;
  bool _direct; // True: ASC; False: DESC
  int _pos;
};

class ExprOrderBy : public BaseExpr {
public:
  ~ExprOrderBy() { delete _vctItem; }
  ExprType GetType() { return ExprType::EXPR_ORDER_BY; }

public:
  MVectorPtr<ExprOrderItem *> *_vctItem;
};

class ExprLimit : public BaseExpr {
public:
  ExprType GetType() { return ExprType::EXPR_LIMIT; }

public:
  // Offset for return rows, default 0. Only valid for root select result.
  int _rowOffset{0};
  // The max rows to return, -1 means return all. Only valid for root select
  // result.
  int _rowCount{-1};
};

enum class LockType { NO_LOCK, SHARE_LOCK, WRITE_LOCK };

// This select class is only for parse, it will convert into a series of
// statement in preprocess.
class ExprSelect : public ExprStatement {
public:
  ExprSelect() {}
  ~ExprSelect() {
    delete _vctCol;
    delete _vctTable;
    delete _exprWhere;
    delete _exprGroupBy;
    delete _exprHaving;
    delete _exprOrderBy delete _exprLimit;
  }
  ExprType GetType() { return ExprType::EXPR_SELECT; }
  bool Preprocess() {
    // TO DO
  }

public:
  // Remove repeated rows or not
  bool _bDistinct{false};
  // The selected columns
  MVectorPtr<ExprResColumn *> *_vctCol{nullptr};
  // The source tables, first table is ExprTable, the following tables are
  // ExprJoinTable if have.
  MVectorPtr<ExprTable *> *_vctTable{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOn *_exprOn(nullptr);

  ExprGroupBy *_exprGroupBy{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit(nullptr);
  LockType _lockType{LockType::NO_LOCK};
};

class ExprInsert : public ExprStatement {
public:
  ExprInsert() {}

  ~ExprInsert() {
    delete _exprTable;
    delete _exprSelect;
    delete _vctCol;
    delete _vctRowData;

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
  MVectorPtr<ExprColumn *> *_vctCol;
  // The values that will be inserted.
  MVectorPtr<MVectorPtr<ExprData *> *> *_vctRowData;
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
  ExprTable *_exprTable(nullptr);
  // The update columns and their values, have saved in ExprColumn
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
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
