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
  ExprCount(ExprData *exprData, bool bStar = false)
      : _exprData(exprData), _bStar(bStar) {
    assert((exprData != nullptr) ^ bStar);
  }
  ~ExprCount() { delete _exprData; }

  ExprType GetType() override { return ExprType::EXPR_COUNT; }
  bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
            IDataValue &dv) override {
    if (_bStar) {
      dv.Add(1L);
      return true;
    } else {
      IDataValue *val = _exprData->Calc(vdParas, vdRow);
      if (val == nullptr)
        return false;

      if (!val->IsNull())
        dv.Add(1L);
      return true;
    }
  }

public:
  ExprData *_exprData;
  bool _bStar;
};

class ExprSum : public ExprAggr {
public:
  ExprSum(ExprData *exprData) : _exprData(exprData) {}
  ~ExprSum() { delete _exprData; }
  ExprType GetType() override { return ExprType::EXPR_SUM; }
  bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
            IDataValue &dv) override {
    IDataValue *val = _exprData->Calc(vdParas, vdRow);
    if (val == nullptr)
      return false;

    if (!val->IsNull())
      dv.Add(val->GetDouble());

    val->DecRef();
    return true;
  }

public:
  ExprData *_exprData;
};

class ExprMax : public ExprAggr {
public:
  ExprMax(ExprData *exprData) : _exprData(exprData) {}
  ~ExprMax() { delete _exprData; }

  ExprType GetType() override { return ExprType::EXPR_MAX; }
  bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
            IDataValue &dv) override {
    IDataValue *val = _exprData->Calc(vdParas, vdRow);
    if (val == nullptr)
      return false;
    if (*val > dv) {
      dv.Copy(*val);
    }
    val->DecRef();
    return true;
  }

public:
  ExprData *_exprData;
};

class ExprMin : public ExprAggr {
public:
  ExprMin(ExprData *exprData) : _exprData(exprData) {}
  ~ExprMin() { delete _exprData; }

  ExprType GetType() override { return ExprType::EXPR_MIN; }
  bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
            IDataValue &dv) override {
    IDataValue *val = _exprData->Calc(vdParas, vdRow);
    if (val == nullptr)
      return false;
    if (*val < dv) {
      dv.Copy(*val);
    }

    val->DecRef();
    return true;
  }

public:
  ExprData *_exprData;
};

class ExprAvg : public ExprAggr {
public:
  ExprAvg(ExprData *exprData) : _exprData(exprData) {}
  ~ExprAvg() { delete _exprData; }

  ExprType GetType() override { return ExprType::EXPR_AVG; }
  bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
            IDataValue &dv) override {
    IDataValue *val = _exprData->Calc(vdParas, vdRow);
    if (val == nullptr)
      return false;

    if (!val->IsNull())
      dv.Add(val->GetDouble());
    return true;
  }

public:
  ExprData *_exprData;
};
} // namespace storage
