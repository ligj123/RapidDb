#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprData.h"
#include "ExprLogic.h"
#include "ExprValue.h"
#include <unordered_set>
#include <vector>

using namespace std;
namespace storage {
enum class TableType {
  PersistTable, // From physical table
  BlockTable,   // The result saved in temp block table
  HashTable     // The result saved in hash table
};

// The column order by
struct OrderCol {
  int colPos;
  bool bAsc;
};

class ExprSelect : public BaseExpr {
public:
protected:
  // The columns and How to get values.
  ExprValueArrayOut *_exprVAout;
  ExprWhere *_where; // Where conditions
  // The columns for order by in _exprVAout
  vector<OrderCol> _vctOrder;
  TableType _sourType;
  TableType _destType;
};

class ExprInnerJoin : public ExprSelect {};

class ExprLeftJoin : public ExprSelect {};

class ExprRightJoin : public ExprSelect {};

class ExprOutterJoin : public ExprSelect {};

class ExprGroupBy : public ExprAggr {
public:
  ExprGroupBy(vector<ExprAggr *> &vctChild)
      : ExprAggr(ExprType::EXPR_GROUP_BY) {
    _vctChild.swap(vctChild);
  }

  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) override {
    for (auto expr : _vctChild) {
      expr->Calc(vdSrc, vdDst);
    }
  }

protected:
  vector<ExprAggr *> _vctChild;
};
} // namespace storage
