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

/**
 * @brief Base class for all expression to calc and return data value.
 */
class ExprData : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  /**Returned DataValue maybe refer to one of value in vdPara or vdRow, or
   * created newly. if created newly, need user to release it.*/
  virtual IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) = 0;
};

/**
 * @brief Corresponding to a column in the table or temp table.
 */
class ExprColumn : public ExprData {
public:
  ExprColumn(int rowPos) : _rowPos(rowPos) {}

  ExprType GetType() { return ExprType::EXPR_COLUMN; }
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return vdRow[_rowPos];
  }

protected:
  int _rowPos; // The position in the table.
};

/*
 * @brief Get Parameter from inputed parameter array.
 */
class ExprParameter : public ExprData {
public:
  ExprParameter(int paraPos) : _paraPos(paraPos) {}

  ExprType GetType() { return ExprType::EXPR_PARAMETER; }
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return vdPara[_paraPos];
  }

protected:
  int _paraPos; // The position in parameter array.
};

class ExprAdd : public ExprData {
public:
  ExprAdd(ExprData *left, ExprData *right)
      : _exprLeft(left), _exprRight(right) {}
  ~ExprAdd() {
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() { return ExprType::EXPR_ADD; }
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsStringType() || right->IsStringType()) {
      StrBuff sb(left->GetDataLength() * 2 + right->GetDataLength() * 2);
      left->ToString(sb);
      right->ToString(sb);
      rt = new DataValueVarChar(sb.GetBuff(), sb.GetBufLen() + 1,
                                sb.GetBufLen() + 1);
    } else if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() + right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() + right->GetDouble());
    } else {
      rt = left->Clone();
    }
    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
  }

protected:
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprSub : public ExprData {
public:
  ExprSub(ExprData *left, ExprData *right)
      : _exprLeft(left), _exprRight(right) {}
  ~ExprSub() {
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() { return ExprType::EXPR_SUB; }
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() - right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() - right->GetDouble());
    } else {
      rt = left->Clone();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
  }

protected:
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprMul : public ExprData {
public:
  ExprMul(ExprData *left, ExprData *right)
      : _exprLeft(left), _exprRight(right) {}
  ~ExprMul() {
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() { return ExprType::EXPR_MUL; }
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() * right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() * right->GetDouble());
    } else {
      rt = left->Clone();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
  }

protected:
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprDiv : public ExprData {
public:
  ExprDiv(ExprData *left, ExprData *right)
      : _exprLeft(left), _exprRight(right) {}
  ~ExprDiv() {
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() { return ExprType::EXPR_DIV; }
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      int64_t r = right->GetLong();
      if (r != 0)
        rt = new DataValueLong(left->GetLong() / r);
    } else if (left->IsDigital() && right->IsDigital()) {
      double r = right->GetDouble();
      if (r != 0)
        rt = new DataValueDouble(left->GetDouble() / r);
    }
    if (rt == nullptr) {
      rt = left->Clone();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
  }

protected:
  ExprData *_exprLeft;
  ExprData *_exprRight;
};
} // namespace storage
