#pragma once
#include "../cache/StrBuff.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../table/Table.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprAggr.h"
#include "ExprData.h"
#include "ExprLogic.h"

#include <unordered_set>

using namespace std;
namespace storage {
class Session;
class PhysTable;
bool FillElemFiled(const MStrHashMap<uint32_t> &mapColPos,
                   MVector<ExprElem *> &vctElem);

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

  bool Preprocess(const MStrHashMap<uint32_t> &mapColPos) {
    MVector<ExprElem *> vctElem;
    vctElem.reserve(32);
    _exprLogic->CollectElem(ExprType::EXPR_FIELD, vctElem);
    if (!FillElemFiled(mapColPos, vctElem)) {
      return false;
    }

    return true;
  }

public:
  ExprLogic *_exprLogic{nullptr};
};

typedef ExprCondition<ExprType::EXPR_ON> ExprOn;
typedef ExprCondition<ExprType::EXPR_HAVING> ExprHaving;

struct IndexValue {
  IndexValue() {}
  IndexValue(IndexValue &&src)
      : _dvLeft(src._dvLeft), _dvRight(src._dvRight), _bRange(src._bRange),
        _bIncLeft(src._bIncLeft), _bIncRight(src._bIncRight),
        _bValid(src._bValid) {
    src._dvLeft = nullptr;
    src._dvRight = nullptr;
  }

  ~IndexValue() {
    assert(_bRange || *_dvLeft == *_dvRight);
    if (_dvLeft != nullptr) {
      _dvLeft->DecRef();
      _dvRight->DecRef();
    }
  }

  IndexValue &operator=(IndexValue &&src) {
    _dvLeft = src._dvLeft;
    _dvRight = src._dvRight;
    _bRange = src._bRange;
    _bIncLeft = src._bIncLeft;
    _bIncRight = src._bIncRight;
    _bValid = src._bValid;
    src._dvLeft = nullptr;
    src._dvRight = nullptr;
    return *this;
  }

  // The left range border
  IDataValue *_dvLeft{nullptr};
  // The right range boder, if _bRange=False, it should euqal right border
  IDataValue *_dvRight{nullptr};
  bool _bRange{true};
  bool _bIncLeft{true};  // Include left border, only valid bRange=TRUE
  bool _bIncRight{true}; // Include right boder, only valid bRange=TRUE
  bool _bValid{true};    // The range is valid or not
};

class IndexCondition {
public:
  IndexCondition() {}
  IndexCondition(IndexCondition &&src)
      : _field(src._field), _vctValue(move(src._vctValue)) {
    src._field = nullptr;
  }
  ~IndexCondition() {}
  IndexCondition &operator=(IndexCondition &&src) {
    _field = src._field;
    src._field = nullptr;
    _vctValue = move(src._vctValue);
    return *this;
  }

public:
  // Copied from UseIndex, NOT need to free
  ExprField *_field{nullptr};
  MVector<IndexValue> _vctValue;
};

// To query physical table, point out which index will be used. Only one index
// can be selected.
class IndexSearch {
public:
  IndexSearch(ExprLogic *idxLogic, int idxPos)
      : _idxLogic(idxLogic), _indexPos(idxPos) {}
  ~IndexSearch() {
    delete _idxLogic;
    delete _vctPointCond;
  }

  void EqualReplace(VectorDataValue &paras);

  // If the primary or secondary index can be used to query, copy the query
  // conditions to here. Only one index can be used. Only valid for physical
  // table select.
  ExprLogic *_idxLogic{nullptr};
  // The point search conditions for an index (include primary and secondary
  // index). The ExprLogic oncly can be ExprComp with CompType::EQ or ExprInNot
  // with _bIn=true
  MVectorPtr<ExprLogic *> *_vctPointCond{nullptr};
  // which index used, The position of index that start from 0(primary key)
  int _indexPos;
  // Point query or not. If true, do not need to optimizate again in new
  // statement's where
  bool _bPointQuery{false};
};

class ExprWhere : public ExprCondition<ExprType::EXPR_WHERE> {
public:
  ExprWhere(ExprLogic *exprLogic) : ExprCondition(exprLogic) {}
  ~ExprWhere() { delete _indexSearch; }

  bool Preprocess(PhysTable *table, const MStrHashMap<uint32_t> &mapColPos);

public:
  // This variable will be set when preprocess
  IndexSearch *_indexSearch{nullptr};
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

  bool Preprocess(const MStrHashMap<uint32_t> &mapColPos);

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

  bool Preprocess(const MStrHashMap<uint32_t> &mapColPos);

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
    delete _exprDestSelect;
  }
  ExprType GetType() override { return ExprType::EXPR_SELECT; }
  bool Preprocess(Database *currDb = nullptr) override;

public:
  // Remove repeated rows or not
  bool _bDistinct{false};
  LockType _lockType{LockType::NO_LOCK};
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
  // The destination statment generate by proprocess
  ExprStatement *_exprDestSelect;
};

class ExprTableSelect : public ExprStatement {
public:
  ~ExprTableSelect() {
    delete _vctCol;
    delete _exprTable;
    delete _exprWhere;
    delete _exprGroupBy;
    delete _exprOrderBy;
    delete _exprLimit;
  }

  ExprType GetType() override { return ExprType::EXPR_TABLE_SELECT; }
  bool Preprocess(Database *currDb = nullptr) override;

public:
  // This select is to return final result or as bottom of join tables.
  bool _bIsolate{true};
  // Remove repeated rows or not
  bool _bDistinct{false};
  // The selected columns
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // The destion table
  ExprTable *_exprTable{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  // Group by expression, only valid _bIsolate=true
  ExprGroupBy *_exprGroupBy{nullptr};
  // Order by expression, only valid _bIsolate=true
  ExprOrderBy *_exprOrderBy{nullptr};
  // Limit exoression, only valid _bIsolate=true
  ExprLimit *_exprLimit{nullptr};
  // Lock type (for update, for read),  only valid _bIsolate=true
  LockType _lockType{LockType::NO_LOCK};
};

class ExprJoinSelect : public ExprStatement {
public:
  ~ExprJoinSelect() {
    delete _vctCol;
    delete _leftTable;
    delete _rightTable;
    delete _exprWhere;
    delete _exprOn;
    delete _exprGroupBy;
    delete _exprOrderBy;
    delete _exprLimit;
  }
  ExprType GetType() override { return ExprType::EXPR_TABLE_SELECT; }
  bool Preprocess(Database *currDb = nullptr) override {
    abort();
    return false;
  }

public:
  // This select
  bool _bTop{true};
  // Remove repeated rows or not
  bool _bDistinct{false};
  // The selected columns
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  ExprTableSelect *_leftTable{nullptr};
  ExprTableSelect *_rightTable{nullptr};
  JoinType _joinType{JoinType::INNER_JOIN};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOn *_exprOn{nullptr};
  ExprGroupBy *_exprGroupBy{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
};

class ExprInsert : public ExprStatement {
public:
  ~ExprInsert() {
    delete _exprTable;
    delete _vctCol;
    delete _vctRowData;
    delete _exprSelect;
  }

  ExprType GetType() override { return ExprType::EXPR_INSERT; }
  bool Preprocess(Database *currDb = nullptr) override;

public:
  // The destion table
  ExprTable *_exprTable{nullptr};
  // The columns that assign values; if empty, it will be filled with all
  // table's columns
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // The multi row data that will be inserted.
  MVectorPtr<MVectorPtr<ExprElem *> *> *_vctRowData{nullptr};
  // The source data that selected from other table and will be inserted into
  // this table
  ExprSelect *_exprSelect{nullptr};
  // True, update if the primary key has exist
  bool _bUpsert{false};
};

class ExprUpdate : public ExprStatement {
public:
  ~ExprUpdate() {
    delete _exprTable;
    delete _vctCol;
    delete _exprWhere;
    delete _exprOrderBy;
    delete _exprLimit;
  }
  ExprType GetType() override { return ExprType::EXPR_UPDATE; }
  bool Preprocess(Database *currDb = nullptr) override;

public:
  // The destion table information
  ExprTable *_exprTable{nullptr};
  // The update columns and their values, have saved in ExprColumn
  MVectorPtr<ExprColumn *> *_vctCol{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
};

class ExprDelete : public ExprStatement {
public:
  ~ExprDelete() {
    delete _exprTable;
    delete _exprWhere;
    delete _exprOrderBy;
    delete _exprLimit;
  }

  ExprType GetType() override { return ExprType::EXPR_DELETE; }
  bool Preprocess(Database *currDb = nullptr) override;

public:
  // The destion table information
  ExprTable *_exprTable{nullptr};
  // Where condition
  ExprWhere *_exprWhere{nullptr};
  ExprOrderBy *_exprOrderBy{nullptr};
  ExprLimit *_exprLimit{nullptr};
};
} // namespace storage
