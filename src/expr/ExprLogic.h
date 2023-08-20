#pragma once
#include "../cache/Mallocator.h"
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprData.h"
#include <unordered_set>

using namespace std;
namespace storage {
enum class CompType { EQ, GT, GE, LT, LE, NE };

class ExprComp : public ExprLogic {
public:
  ExprComp(CompType type, ExprData *left, ExprData *right)
      : _compType(type), _exprLeft(left), _exprRight(right) {}
  ~ExprComp() {
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() { return ExprType::EXPR_COMP; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);

    bool b = false;
    switch (_compType) {
    case CompType::EQ:
      b = (*left == *right);
    case CompType::GT:
      b = (*left > *right);
    case CompType::GE:
      b = (*left >= *right);
    case CompType::LT:
      b = (*left < *right);
    case CompType::LE:
      b = (*left <= *right);
    case CompType::NE:
      b = (*left != *right);
    default:
      abort();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return b;
  }

public:
  CompType _compType;
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprInNot : public ExprLogic {
public:
  ExprInNot(ExprField *exprField, ExprArray *exprArray, bool bIn = true)
      : _exprField(exprField), _exprArray(exprArray) {}
  ~ExprInNot() {
    delete _exprField;
    delete _exprArray;
  }

  ExprType GetType() { return ExprType::EXPR_IN_OR_NOT; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _exprField->Calc(vdPara, vdRow);
    bool b = _exprArray->Exist(pdv);
    if (!pdv->IsReuse())
      delete pdv;

    return (_bIn ? b : !b);
  }

protected:
  ExprField *_exprField;
  ExprArray *_exprArray;
  bool _bIn;
};

class ExprIsNullNot : public ExprLogic {
public:
  ExprIsNullNot(ExprData *child, bool bNull) : _child(child), _bNull(bNull) {}
  ~ExprIsNullNot() { delete _child; }

  ExprType GetType() { return ExprType::EXPR_IS_NULL_NOT; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _child->Calc(vdPara, vdRow);
    bool b = pdv->IsNull();
    if (!pdv->IsReuse())
      delete pdv;
    return (_bNull ? b : !b);
  }

public:
  ExprData *_child;
  bool _bNull;
};

class ExprBetween : public ExprLogic {
public:
  ExprBetween(ExprData *child, ExprData *left, ExprData *right)
      : _child(child), _exprLeft(left), _exprRight(right) {}
  ~ExprBetween() {
    delete _child;
    delete _exprLeft;
    delete _exprRight;
  }

  ExprType GetType() { return ExprType::EXPR_BETWEEN; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _child->Calc(vdPara, vdRow);
    bool b =
        (*pdv >= *_exprLeft->GetValue() && *pdv <= *_exprRight->GetValue());

    if (!pdv->IsReuse())
      delete pdv;
    return b;
  }

public:
  ExprData *_child;
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprLike : public ExprLogic {
public:
  ExprLike(ExprData *exprData, ExprConst *exprPatten)
      : _child(exprData), _exprPatten(exprPatten) {
    assert(exprPatten->GetValue()->IsStringType());
  }
  ~ExprLike() {
    delete _child;
    delete _exprPatten;
  }

  ExprType GetType() { return ExprType::EXPR_LIKE; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    // TO DO
    return true;
  }

public:
  ExprData *_child;
  ExprConst *_exprPatten;
};

class ExprNot : public ExprLogic {
public:
  ExprNot(ExprLogic *child) : _child(child) {}
  ~ExprNot() { delete _child; }

  ExprType GetType() { return ExprType::EXPR_NOT; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return !_child->Calc(vdPara, vdRow);
  }

public:
  ExprLogic *_child;
};

class ExprAnd : public ExprLogic {
public:
  ExprAnd(MVector<ExprLogic *> &vctChild) { _vctChild.swap(vctChild); }
  ~ExprAnd() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  ExprType GetType() { return ExprType::EXPR_AND; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    for (ExprLogic *expr : _vctChild) {
      if (!expr->Calc(vdPara, vdRow))
        return false;
    }

    return true;
  }

public:
  MVector<ExprLogic *> _vctChild;
};

class ExprOr : public ExprLogic {
public:
  ExprOr(MVector<ExprLogic *> vctChild) { _vctChild.swap(vctChild); }
  ~ExprOr() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  ExprType GetType() { return ExprType::EXPR_OR; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    for (ExprLogic *expr : _vctChild) {
      if (expr->Calc(vdPara, vdRow))
        return true;
    }
    return false;
  }

public:
  MVector<ExprLogic *> _vctChild;
};

} // namespace storage
