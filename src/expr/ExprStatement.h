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
  ExprSelect(BaseTable *sourTable, BaseTable *destTable,
             ExprValueArrayOut *exprVAout, ExprCondition *where,
             MVector<OrderCol>::Type &vctOrder, bool bDistinct,
             bool bCacheResult, VectorDataValue &vctPara, bool statTime = false)
      : _sourTable(sourTable), _destTable(destTable), _exprVAout(exprVAout),
        _where(where), _bDistinct(bDistinct), _bCacheResult(bCacheResult),
        _bStatTime(statTime) {
    _vctOrder.swap(vctOrder);
    _vctPara.swap(vctPara);
  }
  ~ExprSelect() {
    delete _sourTable;
    delete _destTable;
    delete _exprVAout;
    delete _where;
  }

  ExprType GetType() { return ExprType::EXPR_SELECT; }
  const BaseTable *GetSourTable() const { return _sourTable; }
  const BaseTable *GetDestTable() const { return _destTable; }
  const ExprValueArrayOut *GetExprVAout() const { return _exprVAout; }
  const ExprCondition *GetWhere() const { return _where; }
  const MVector<OrderCol>::Type GetVctOrder() const { return _vctOrder; }
  bool IsDistinct() const { return _bDistinct; }
  bool IsCacheResult() const { return _bCacheResult; }
  const VectorDataValue &GetParameters() { return _vctPara; }
  bool IsStatTime() { return _bStatTime; }

protected:
  // The source table information.
  BaseTable *_sourTable;
  // The result table information.
  BaseTable *_destTable;
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
  // Used to save parameters
  VectorDataValue _vctPara;
  // if record time for statistics
  bool _bStatTime;
};

class ExprJoin : public ExprSelect {
public:
  ExprJoin(BaseTable *sourTable, BaseTable *destTable,
           ExprValueArrayOut *exprVAout, ExprCondition *where,
           MVector<OrderCol>::Type &vctOrder, bool bDistinct, bool bCacheResult,
           BaseTable *rightTable, ExprOn *exprOn, JoinType jType,
           VectorDataValue &vctPara, bool statTime = false)
      : ExprSelect(sourTable, destTable, exprVAout, where, vctOrder, bDistinct,
                   bCacheResult, vctPara, statTime),
        _rightTable(rightTable), _exprOn(exprOn), _joinType(jType) {}
  ~ExprJoin() {
    delete _rightTable;
    delete _exprOn;
  }

  ExprType GetType() { return ExprType::EXPR_JOIN; }
  const BaseTable *GetRightTable() const { return _rightTable; }
  const ExprOn *GetExprOn() const { return _exprOn; }
  JoinType GetJoinType() { return _joinType; }

protected:
  // The right source table information for join
  BaseTable *_rightTable;
  // On condition for join
  ExprOn *_exprOn;
  // Join type
  JoinType _joinType;
};

class ExprGroupBy : public ExprSelect {
public:
  ExprGroupBy(BaseTable *sourTable, BaseTable *destTable,
              MVector<ExprAggr *>::Type &vctAggr, ExprCondition *where,
              MVector<OrderCol>::Type &vctOrder, bool bDistinct,
              bool bCacheResult, ExprCondition *having,
              VectorDataValue &vctPara, bool statTime = false)
      : ExprSelect(sourTable, destTable, nullptr, where, vctOrder, bDistinct,
                   bCacheResult, vctPara, statTime),
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
  ExprInsert(PhysTable *tableDesc, ExprValueArrayIn *exprVAin,
             ExprSelect *exprSelect, VectorDataValue &vctPara,
             bool bUpsert = false, bool statTime = false)
      : _tableDesc(tableDesc), _exprVAin(exprVAin), _exprSelect(exprSelect),
        _bUpsert(bUpsert), _bStatTime(statTime) {
    _vctPara.swap(vctPara);
  }
  ExprInsert(PhysTable *tableDesc, ExprValueArrayIn *exprVAin,
             VectorDataValue &vctPara, bool bUpsert = false,
             bool statTime = false)
      : _tableDesc(tableDesc), _exprVAin(exprVAin), _bUpsert(bUpsert),
        _bStatTime(statTime), _exprSelect(nullptr) {
    _vctPara.swap(vctPara);
  }
  ~ExprInsert() {
    delete _exprVAin;
    delete _exprSelect;
  }

  ExprType GetType() { return ExprType::EXPR_INSERT; }
  PhysTable *GetTableDesc() { return _tableDesc; }
  ExprValueArrayIn *GetExprValueArrayIn() { return _exprVAin; }
  ExprSelect *GetExprSelect() { return _exprSelect; }
  const VectorDataValue &GetParameters() { return _vctPara; }
  bool IsStatTime() { return _bStatTime; }
  bool IsUpsert() { return _bUpsert; }

protected:
  // The destion persistent table, managed by database, can not delete here.
  PhysTable *_tableDesc;
  ExprValueArrayIn *_exprVAin;
  ExprSelect *_exprSelect;
  // Used to save parameters
  VectorDataValue _vctPara;
  // if record time for statistics
  bool _bStatTime;
  // True, update if the key has exist
  bool _bUpsert;
};

class ExprUpdate : public BaseExpr {
public:
  ExprUpdate(PhysTable *tableDesc, ExprValueArrayIn *exprVAin,
             ExprCondition *where, VectorDataValue &vctPara,
             bool statTime = false)
      : _tableDesc(tableDesc), _exprVAin(exprVAin), _where(where),
        _bStatTime(statTime) {
    _vctPara.swap(vctPara);
  }
  ~ExprUpdate() {
    delete _exprVAin;
    delete _where;
  }

  ExprType GetType() { return ExprType::EXPR_UPDATE; }
  PhysTable *GetTableDesc() { return _tableDesc; }
  ExprValueArrayIn *GetExprValueArrayIn() { return _exprVAin; }
  ExprCondition *GetWhere() { return _where; }
  const VectorDataValue &GetParameters() { return _vctPara; }
  bool IsStatTime() { return _bStatTime; }

protected:
  // The destion persistent table, managed by database, can not delete here.
  PhysTable *_tableDesc;
  // The expression that how to get values
  ExprValueArrayIn *_exprVAin;
  // Where condition
  ExprCondition *_where;
  // Used to save parameters
  VectorDataValue _vctPara;
  // if record time for statistics
  bool _bStatTime;
};

class ExprDelete : public BaseExpr {
public:
  ExprDelete(PhysTable *tableDesc, ExprCondition *where,
             VectorDataValue &vctPara, bool statTime = false)
      : _tableDesc(tableDesc), _where(where), _bStatTime(statTime) {
    _vctPara.swap(vctPara);
  }
  ~ExprDelete() { delete _where; }

  ExprType GetType() { return ExprType::EXPR_DELETE; }
  PhysTable *GetTableDesc() { return _tableDesc; }
  ExprCondition *GetWhere() { return _where; }
  const VectorDataValue &GetParameters() { return _vctPara; }
  bool IsStatTime() { return _bStatTime; }

protected:
  // The destion persistent table, managed by database, can not delete here.
  PhysTable *_tableDesc;
  // Where condition
  ExprCondition *_where;
  // Used to save parameters
  VectorDataValue _vctPara;
  // if record time for statistics
  bool _bStatTime;
};
} // namespace storage
