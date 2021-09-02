#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../table/TableDesc.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprData.h"
#include "ExprLogic.h"
#include "ExprValue.h"
#include <unordered_set>

using namespace std;
namespace storage {
enum class TableType {
  PersistTable, // From physical table
  CacheTable,   // The result saved in temp block table
  HashTable     // The result saved in hash table
};

enum class JoinType { INNER_JOIN, LEFT_JOIN, RIGHT_JOIN, OUTTER_JOIN };

// The column order by
struct OrderCol {
  int colPos;
  bool bAsc;
};

class ExprSelect : public BaseExpr {
public:
  ExprSelect(BaseTableDesc *sourTable, BaseTableDesc *destTable,
             ExprValueArrayOut *exprVAout, ExprCondition *where,
             MVector<OrderCol>::Type &vctOrder, bool bDistinct,
             bool bCacheResult)
      : _sourTable(sourTable), _destTable(destTable), _exprVAout(exprVAout),
        _where(where), _bDistinct(bDistinct), _bCacheResult(bCacheResult) {
    _vctOrder.swap(vctOrder);
  }
  ~ExprSelect() {
    delete _sourTable;
    delete _destTable;
    delete _exprVAout;
    delete _where;
  }

  ExprType GetType() { return ExprType::EXPR_SELECT; }
  const BaseTableDesc *GetSourTable() const { return _sourTable; }
  const BaseTableDesc *GetDestTable() const { return _destTable; }
  const ExprValueArrayOut *GetExprVAout() const { return _exprVAout; }
  const ExprCondition *GetWhere() const { return _where; }
  const MVector<OrderCol>::Type GetVctOrder() const { return _vctOrder; }
  bool IsDistinct() const { return _bDistinct; }
  bool IsCacheResult() const { return _bCacheResult; }

protected:
  // The source table information.
  BaseTableDesc *_sourTable;
  // The result table information.
  BaseTableDesc *_destTable;
  // The columns and How to get values.
  ExprValueArrayOut *_exprVAout;
  // Where conditions
  ExprCondition *_where;
  // The columns for order by in _exprVAout
  MVector<OrderCol>::Type _vctOrder;
  // If the result need to be remove duplicate
  bool _bDistinct;
  // If save the result to cache and reused next time
  bool _bCacheResult;
};

class ExprJoin : public ExprSelect {
public:
  ExprJoin(BaseTableDesc *sourTable, BaseTableDesc *destTable,
           ExprValueArrayOut *exprVAout, ExprCondition *where,
           MVector<OrderCol>::Type &vctOrder, bool bDistinct, bool bCacheResult,
           BaseTableDesc *rightTable, ExprOn *exprOn, JoinType jType)
      : ExprSelect(sourTable, destTable, exprVAout, where, vctOrder, bDistinct,
                   bCacheResult),
        _rightTable(rightTable), _exprOn(exprOn), _joinType(jType) {}
  ~ExprJoin() {
    delete _rightTable;
    delete _exprOn;
  }

  ExprType GetType() { return ExprType::EXPR_JOIN; }
  const BaseTableDesc *GetRightTable() const { return _rightTable; }
  const ExprOn *GetExprOn() const { return _exprOn; }
  JoinType GetJoinType() { return _joinType; }

protected:
  // The right source table information for join
  BaseTableDesc *_rightTable;
  // On condition for join
  ExprOn *_exprOn;
  // Join type
  JoinType _joinType;
};

class ExprGroupBy : public ExprSelect {
public:
  ExprGroupBy(BaseTableDesc *sourTable, BaseTableDesc *destTable,
              MVector<ExprAggr *>::Type &vctAggr, ExprCondition *where,
              MVector<OrderCol>::Type &vctOrder, bool bDistinct,
              bool bCacheResult, ExprCondition *having)
      : ExprSelect(sourTable, destTable, nullptr, where, vctOrder, bDistinct,
                   bCacheResult),
        _having(having) {
    _vctChild.swap(vctAggr);
  }
  ~ExprGroupBy() {
    for (auto child : _vctChild)
      delete child;

    _vctChild.clear();
    delete _having;
  }

  ExprType GetType() { return ExprType::EXPR_GROUP_BY; }
  MVector<ExprAggr *>::Type GetVctChild() { return _vctChild; }
  ExprCondition *GetHaving() { return _having; }

protected:
  // This variable will replace _exprVAout to calc the values.
  MVector<ExprAggr *>::Type _vctChild;
  ExprCondition *_having;
};

class ExprInsert : public BaseExpr {
public:
  ExprInsert(PersistTableDesc *tableDesc, ExprValueArrayIn *exprVAin,
             ExprSelect *exprSelect)
      : _tableDesc(tableDesc), _exprVAin(exprVAin), _exprSelect(exprSelect) {}
  ExprInsert(PersistTableDesc *tableDesc, ExprValueArrayIn *exprVAin)
      : _tableDesc(tableDesc), _exprVAin(exprVAin) {}
  ~ExprInsert() {
    delete _exprVAin;
    delete _exprSelect;
  }

  ExprType GetType() { return ExprType::EXPR_INSERT; }
  PersistTableDesc *GetTableDesc() { return _tableDesc; }
  ExprValueArrayIn *GetExprValueArrayIn() { return _exprVAin; }
  ExprSelect *GetExprSelect() { return _exprSelect; }

protected:
  // The destion persistent table, managed by database, can not delete here.
  PersistTableDesc *_tableDesc;
  ExprValueArrayIn *_exprVAin;
  ExprSelect *_exprSelect;
};

class ExprUpdate : public BaseExpr {
public:
  ExprUpdate(PersistTableDesc *tableDesc, ExprValueArrayIn *exprVAin,
             ExprCondition *where)
      : _tableDesc(tableDesc), _exprVAin(exprVAin), _where(where) {}
  ~ExprUpdate() {
    delete _exprVAin;
    delete _where;
  }

  ExprType GetType() { return ExprType::EXPR_UPDATE; }
  PersistTableDesc *GetTableDesc() { return _tableDesc; }
  ExprValueArrayIn *GetExprValueArrayIn() { return _exprVAin; }
  ExprCondition *GetWhere() { return _where; }

protected:
  // The destion persistent table, managed by database, can not delete here.
  PersistTableDesc *_tableDesc;
  // The expression that how to get values
  ExprValueArrayIn *_exprVAin;
  // Where condition
  ExprCondition *_where;
};

class ExprDelete : public BaseExpr {
public:
  ExprDelete(PersistTableDesc *tableDesc, ExprCondition *where)
      : _tableDesc(tableDesc), _where(where) {}
  ~ExprDelete() { delete _where; }

  ExprType GetType() { return ExprType::EXPR_DELETE; }
  PersistTableDesc *GetTableDesc() { return _tableDesc; }
  ExprCondition *GetWhere() { return _where; }

protected:
  // The destion persistent table, managed by database, can not delete here.
  PersistTableDesc *_tableDesc;
  // Where condition
  ExprCondition *_where;
};
} // namespace storage
