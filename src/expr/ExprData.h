#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueNull.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include <unordered_set>

using namespace std;
namespace storage {
/**
 * @brief save const value for expression
 */
class ExprConst : public ExprData {
public:
  ExprConst(IDataValue *val) : _val(val) { _val->SetConstRef(); }
  ExprConst(int64_t ival) {
    _val = new DataValueLong(ival);
    _val->SetConstRef();
  }
  ExprConst(double dval) {
    _val = new DataValueDouble(dval);
    _val->SetConstRef();
  }
  ExprConst(MString *sval) {
    _val = new DataValueVarChar(sval->c_str(), sval->size());
    _val->SetConstRef();
  }
  ExprConst(bool bval) {
    _val = new DataValueBool(bval);
    _val->SetConstRef();
  }
  ExprConst() {
    _val = new DataValueNull();
    _val->SetConstRef();
  }
  ~ExprConst() { _val->DecRef(); }
  ExprType GetType() override { return ExprType::EXPR_CONST; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    return _val;
  }

  bool IsNull() {
    return _val == nullptr || _val->GetDataType() == DataType::VAL_NULL;
  }

public:
  IDataValue *_val;
};

/**
 * @brief return a column value in the destion table for insert or update.
 */
class ExprField : public ExprData {
public:
  ExprField(MString *tableName, MString *colName)
      : _tableName(tableName), _colName(colName) {}
  ~ExprField() {
    delete _tableName;
    delete _colName;
  }

  ExprType GetType() override { return ExprType::EXPR_FIELD; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    return vdRow[_rowPos];
  }

public:
  MString *_tableName; // The table name this field belong to
  MString *_colName;   // The field name (The related column)
  int _rowPos;         // The position in the table.
};

/*
 * @brief Get Parameter from inputed parameter array.
 */
class ExprParameter : public ExprData {
public:
  ExprType GetType() override { return ExprType::EXPR_PARAMETER; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    return vdParas[_paraPos];
  }

public:
  int _paraPos{-1}; // The position in parameter array.
};

class ExprAdd : public ExprData {
public:
  ExprAdd(ExprData *left, ExprData *right)
      : _exprLeft(left), _exprRight(right) {}
  ~ExprAdd() {
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() override { return ExprType::EXPR_ADD; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdParas, vdRow);
    IDataValue *right = _exprRight->Calc(vdParas, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsStringType() || right->IsStringType()) {
      StrBuff sb(left->GetDataLength() * 2 + right->GetDataLength() * 2);
      left->ToString(sb);
      right->ToString(sb);
      rt = new DataValueVarChar(sb.GetBuff(), sb.GetBufLen() + 1);
    } else if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() + right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() + right->GetDouble());
    } else {
      rt = left->Clone();
    }
    left->DecRef();
    right->DecRef();
    return rt;
  }

public:
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

  ExprType GetType() override { return ExprType::EXPR_SUB; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdParas, vdRow);
    IDataValue *right = _exprRight->Calc(vdParas, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() - right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() - right->GetDouble());
    } else {
      rt = left->Clone();
    }

    left->DecRef();
    right->DecRef();
    return rt;
  }

public:
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

  ExprType GetType() override { return ExprType::EXPR_MUL; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdParas, vdRow);
    IDataValue *right = _exprRight->Calc(vdParas, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() * right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() * right->GetDouble());
    } else {
      rt = left->Clone();
    }

    left->DecRef();
    right->DecRef();
    return rt;
  }

public:
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

  ExprType GetType() override { return ExprType::EXPR_DIV; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdParas, vdRow);
    IDataValue *right = _exprRight->Calc(vdParas, vdRow);
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

    left->DecRef();
    right->DecRef();
    return rt;
  }

public:
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprMinus : public ExprData {

public:
  ExprMinus(ExprData *data) : _exprData(data) {}
  ~ExprMinus() { delete _exprData; }

  ExprType GetType() override { return ExprType::EXPR_MINUS; }
  IDataValue *Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *data = _exprData->Calc(vdParas, vdRow);
    IDataValue *rt = nullptr;

    if (data->IsAutoPrimaryKey()) {
      rt = new DataValueLong(data->GetLong());
    } else if (data->IsDigital()) {
      double r = data->GetDouble();
      if (r != 0)
        rt = new DataValueDouble(data->GetDouble());
    }

    data->DecRef();
    return rt;
  }

public:
  ExprData *_exprData;
};
} // namespace storage
