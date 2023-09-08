#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include <unordered_set>

using namespace std;
namespace storage {
class ExprCount : public ExprAggr {
public:
  ExprCount(int index, int colPos)
      : _index(index), _colPos(colPos), _bAsterisk(false) {}
  ExprCount(int index, bool bAsterisk)
      : _index(index), _colPos(-1), _bAsterisk(bAsterisk) {}

  ExprType GetType() { return ExprType::EXPR_COUNT; }
  void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    DataValueLong dv;
    dv.SetValue(1L);
    if (_bAsterisk) {
      vdRow[_index]->Add(dv);
    } else if (!vdParas[_colPos]->IsNull()) {
      vdRow[_index]->Add(dv);
    }
  }

public:
  // The position for this expr in expr array, the position in the value array
  // too
  int _index;
  // If _bAsterisk == false, it will the position in source row data.
  // If _bAsterisk == true, it will not be used.
  int _colPos;
  // If true, it will be count(*)
  bool _bAsterisk;
};

class ExprSum : public ExprAggr {
public:
  ExprSum(int index, int colPos) : _index(index), _colPos(colPos) {}

  ExprType GetType() { return ExprType::EXPR_SUM; }
  void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *pdv = vdParas[_colPos];
    if (!pdv->IsNull()) {
      vdRow[_index]->Add(*pdv);
    }
  }

public:
  int _index;
  int _colPos;
};

class ExprMax : public ExprAggr {
public:
  ExprMax(int index, int colPos) : _index(index), _colPos(colPos) {}

  ExprType GetType() { return ExprType::EXPR_MAX; }
  void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    if (!vdParas[_colPos]->IsNull() && *vdRow[_index] < *vdParas[_colPos]) {
      vdRow[_index]->Copy(*vdParas[_colPos]);
    }
  }

public:
  int _index;
  int _colPos;
};

class ExprMin : public ExprAggr {
public:
  ExprMin(int index, int colPos, int countPos)
      : _index(index), _colPos(colPos) {}

  ExprType GetType() { return ExprType::EXPR_MIN; }
  void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    if (!vdParas[_colPos]->IsNull() && *vdRow[_index] > *vdParas[_colPos]) {
      vdRow[_index]->Copy(*vdParas[_colPos]);
    }
  }

public:
  int _index;
  int _colPos;
};

class ExprAvg : public ExprAggr {
public:
  ExprAvg(int index, int colPos, int countPos)
      : _index(index), _colPos(colPos), _countPos(countPos) {}

  ExprType GetType() { return ExprType::EXPR_AVG; }
  void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    if (!vdParas[_colPos]->IsNull()) {
      vdRow[_index]->Add(*vdParas[_colPos]);
      DataValueLong dv;
      dv.SetValue(1L);
      vdRow[vdDst.size() - 1]->Add(dv);
    }
  }

public:
  int _index;
  int _colPos;
  int _countPos;
};
} // namespace storage
