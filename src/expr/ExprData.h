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
 * @brief Corresponding to a column in the table or temp table.
 */
class ExprColumn : public ExprData {
public:
  ExprColumn(int rowPos) : ExprData(ExprType::EXPR_COLUMN), _rowPos(rowPos) {}
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
  ExprParameter(int paraPos)
      : ExprData(ExprType::EXPR_PARAMETER), _paraPos(paraPos) {}
  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return vdRow[_paraPos];
  }

protected:
  int _paraPos; // The position in parameter array.
};

class ExprAdd : public ExprData {
public:
  ExprAdd(ExprData *left, ExprData *right)
      : ExprData(ExprType::EXPR_ADD), _exprLeft(left), _exprRight(right) {}
  ~ExprAdd() {
    delete _exprLeft;
    delete _exprRight;
  }

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
      : ExprData(ExprType::EXPR_SUB), _exprLeft(left), _exprRight(right) {}
  ~ExprSub() {
    delete _exprLeft;
    delete _exprRight;
  }

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
      : ExprData(ExprType::EXPR_MUL), _exprLeft(left), _exprRight(right) {}
  ~ExprMul() {
    delete _exprLeft;
    delete _exprRight;
  }

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
      : ExprData(ExprType::EXPR_DIV), _exprLeft(left), _exprRight(right) {}
  ~ExprDiv() {
    delete _exprLeft;
    delete _exprRight;
  }

  IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      int64_t r = right->GetLong();
      if (r != 0)
        rt = new DataValueLong(left->GetLong() * r);
    } else if (left->IsDigital() && right->IsDigital()) {
      double r = right->GetDouble();
      if (r != 0)
        rt = new DataValueDouble(left->GetDouble() * r);
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
