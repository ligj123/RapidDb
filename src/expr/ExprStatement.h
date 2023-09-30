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
  ExprCondition(ExprLogic *exprLogic) : _exprLogic(exprLogic) {}
  ~ExprCondition() { delete _exprLogic; }
  ExprType GetType() override { return ET; }

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
class UseIndex {
public:
  ~UseIndex() { delete _indexExpr; }
  // If the primary or secondary index can be used to query, copy the query
  // conditions to here. Only one index can be used. Only valid for physical
  // table select
  ExprLogic *_indexExpr{nullptr};
  // which index used, The position of index that start from 0(primary key)
  int _indexPos{-1};
};

class ExprWhere : public ExprCondition<ExprType::EXPR_WHERE> {
public:
  ExprWhere(ExprLogic *exprLogic) : ExprCondition(exprLogic) {}
  ~ExprWhere() { delete _useIndex; }

public:
  // This variable will be set when preprocess
  UseIndex *_useIndex{nullptr};
};

class ExprGroupBy : public BaseExpr {
public:
  ExprGroupBy(MVectorPtr<MString *> *vctColName, ExprHaving *exprHaving)
      : _vctColName(vctColName), _exprHaving(exprHaving) {}
  ~ExprGroupBy() {
    delete _vctColName;
    delete _exprHaving;
  }
  ExprType GetType() override { return ExprType::EXPR_GROUP_BY; }

public:
  MVectorPtr<MString *> *_vctColName;
  MVector<int> _vctColPos;
  ExprHaving *_exprHaving;
};

class ExprOrderItem : public BaseExpr {
public:
  ExprOrderItem(MString *colName, bool direct)
      : _colName(colName), _direct(direct) {}
  ExprType GetType() override { return ExprType::EXPR_ORDER_ITEM; }
  ~ExprOrderItem() { delete _colName; }

public:
  MString *_colName;
  bool _direct; // True: ASC; False: DESC
  int _pos{-1};
};

class ExprOrderBy : public BaseExpr {
public:
  ExprOrderBy(MVectorPtr<ExprOrderItem *> *vctItem) : _vctItem(vctItem) {}
  ~ExprOrderBy() { delete _vctItem; }
  ExprType GetType() override { return ExprType::EXPR_ORDER_BY; }

public:
  MVectorPtr<ExprOrderItem *> *_vctItem;
};

class ExprLimit : public BaseExpr {
public:
  ExprLimit(int rowOffset, int rowCount)
      : _rowOffset(rowOffset), _rowCount(rowCount) {}
  ExprType GetType() override { return ExprType::EXPR_LIMIT; }

public:
  // Offset for return rows, default 0. Only valid for root select result.
  int _rowOffset;
  // The max rows to return, -1 means return all. Only valid for root select
  // result.
  int _rowCount;
};

// This select class is only for parse, it will convert into a series of
// statement in preprocess.
class ExprSelect : public ExprStatement {
public:
  ~ExprSelect() {
    delete _vctCol;
    delete _vctTable;
    delete _exprWhere;
    delete _exprOn;
    delete _exprGroupBy;
    delete _exprOrderBy;
    delete _exprLimit;
  }
  ExprType GetType() override { return ExprType::EXPR_SELECT; }
  bool Preprocess() override {
    // TO DO
    return false;
  }

public:
  // Remove repeated rows or not
  bool _bDistinct{false};
  // The selected columns
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // The source tables, first table is ExprTable, the following tables are
  // ExprTable with join type if have.
  MVectorPtr<ExprTable *> *_vctTable{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOn *_exprOn{nullptr};

  ExprGroupBy *_exprGroupBy{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
  LockType _lockType{LockType::NO_LOCK};
};

class ExprInsert : public ExprStatement {
public:
  ~ExprInsert() {
    delete _exprTable;
    delete _vctCol;
    delete _rowData;
    delete _vctRowData;
    delete _exprSelect;

    if (_physTable != nullptr)
      _physTable->DecRef();
  }

  ExprType GetType() override { return ExprType::EXPR_INSERT; }
  bool Preprocess() override {
    // TO DO
    return false;
  }

public:
  // The destion table
  ExprTable *_exprTable{nullptr};
  // The columns that assign values; if empty, it will be filled with all
  // table's columns
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // One row data to insert
  MVectorPtr<ExprElem *> *_rowData{nullptr};
  // The multi row data that will be inserted.
  MVectorPtr<MVectorPtr<ExprElem *> *> *_vctRowData{nullptr};
  // The source data that selected from other table and will be inserted into
  // this table
  ExprSelect *_exprSelect{nullptr};
  // True, update if the primary key has exist
  bool _bUpsert{false};
  // The physical table will insert into. Filled when preprocess
  PhysTable *_physTable{nullptr};
};

class ExprUpdate : public ExprStatement {
public:
  ExprUpdate() {}

  ~ExprUpdate() {
    delete _exprTable;
    delete _vctCol;
    delete _exprWhere;
    delete _exprOrderBy;
    delete _exprLimit;

    if (_physTable != nullptr)
      _physTable->DecRef();
  }

  ExprType GetType() override { return ExprType::EXPR_UPDATE; }
  bool Preprocess() override {
    // TO DO
    return false;
  }

public:
  // The destion table information
  ExprTable *_exprTable{nullptr};
  // The update columns and their values, have saved in ExprColumn
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
  // The physical table will insert into. Filled when preprocess
  PhysTable *_physTable{nullptr};
};

class ExprDelete : public ExprStatement {
public:
  ~ExprDelete() {
    delete _exprTable;
    delete _exprWhere;
    delete _exprOrderBy;
    delete _exprLimit;

    if (_physTable != nullptr)
      _physTable->DecRef();
  }

  ExprType GetType() override { return ExprType::EXPR_DELETE; }
  bool Preprocess() override {
    // TO DO
    return false;
  }

public:
  // The destion table information
  ExprTable *_exprTable{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
  // The physical table will update. Filled when preprocess
  PhysTable *_physTable{nullptr};
};
} // namespace storage
