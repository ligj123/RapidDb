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
  ~ExprCount() {
    delete _exprData;
    delete _exprStart;
  }

  ExprType GetType() { return ExprType::EXPR_COUNT; }
  bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
            IDataValue &dv) override {
    if (_bStar) {
      dv.Add(1);
      return true;
    } else {
      IDataValue *val = _exprData->Calc(vdParas, vdRow);
      if (val == nullptr)
        return false;

      if (!val->IsNull())
        dv.Add(1);
      return true;
    }

  public:
    ExprData *_exprData{nullptr};
    bool _bStar{false};
  };

  class ExprSum : public ExprAggr {
  public:
    ~ExprSum() { delete _exprData; }
    ExprType GetType() { return ExprType::EXPR_SUM; }
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
    ExprData *_exprData{nullptr};
  };

  class ExprMax : public ExprAggr {
  public:
    ~ExprMax() { delete _exprData; }

    ExprType GetType() { return ExprType::EXPR_MAX; }
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
    ExprData *_exprData{nullptr};
  };

  class ExprMin : public ExprAggr {
  public:
    ~ExprMin() { delete _exprData; }

    ExprType GetType() { return ExprType::EXPR_MIN; }
    void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
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
    ExprData *_exprData{nullptr};
  };

  class ExprAvg : public ExprAggr {
  public:
    ~ExprAvg() { delete _exprData; }

    ExprType GetType() { return ExprType::EXPR_AVG; }
    void Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
              IDataValue &dv) override {
      IDataValue *val = _exprData->Calc(vdParas, vdRow);
      if (val == nullptr)
        return false;

      if (!val->IsNull())
        dv.Add(val->GetDouble());
      return true;
    }

  public:
    ExprData *_exprData{nullptr};
  };
} // namespace storage
