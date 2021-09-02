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
  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) override {
    DataValueLong dv(1L, false);
    if (_bAsterisk) {
      vdDst[_index]->Add(dv);
    } else if (!vdSrc[_colPos]->IsNull()) {
      vdDst[_index]->Add(dv);
    }
  }

protected:
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
  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) override {
    IDataValue *pdv = vdSrc[_colPos];
    if (!pdv->IsNull()) {
      vdDst[_index]->Add(*pdv);
    }
  }

protected:
  int _index;
  int _colPos;
};

class ExprMax : public ExprAggr {
public:
  ExprMax(int index, int colPos) : _index(index), _colPos(colPos) {}

  ExprType GetType() { return ExprType::EXPR_MAX; }
  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) override {
    if (!vdSrc[_colPos]->IsNull() && *vdDst[_index] < *vdSrc[_colPos]) {
      vdDst[_index]->Copy(*vdSrc[_colPos]);
    }
  }

protected:
  int _index;
  int _colPos;
};

class ExprMin : public ExprAggr {
public:
  ExprMin(int index, int colPos, int countPos)
      : _index(index), _colPos(colPos) {}

  ExprType GetType() { return ExprType::EXPR_MIN; }
  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) override {
    if (!vdSrc[_colPos]->IsNull() && *vdDst[_index] > *vdSrc[_colPos]) {
      vdDst[_index]->Copy(*vdSrc[_colPos]);
    }
  }

protected:
  int _index;
  int _colPos;
};

class ExprAvg : public ExprAggr {
public:
  ExprAvg(int index, int colPos, int countPos)
      : _index(index), _colPos(colPos), _countPos(countPos) {}

  ExprType GetType() { return ExprType::EXPR_AVG; }
  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) override {
    if (!vdSrc[_colPos]->IsNull()) {
      vdDst[_index]->Add(*vdSrc[_colPos]);
      DataValueLong dv(1L, false);
      vdDst[vdDst.size() - 1]->Add(dv);
    }
  }

protected:
  int _index;
  int _colPos;
  int _countPos;
};
} // namespace storage
