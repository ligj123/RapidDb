#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataValueFactory.h"
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

  ExprType GetType() override { return ExprType::EXPR_COMP; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdParas, vdRow);
    IDataValue *right = _exprRight->Calc(vdParas, vdRow);
    if (left == nullptr || right == nullptr) {
      if (left != nullptr)
        left->DecRef();
      else
        right->DecRef();
      return TriBool::Error;
    }

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

    left->DecRef();
    right->DecRef();
    return b ? TriBool::True : TriBool::False;
  }

public:
  CompType _compType;
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprInNot : public ExprLogic {
public:
  ExprInNot(ExprData *exprData, ExprArray *exprArray, bool bIn = true)
      : _exprData(exprData), _exprArray(exprArray) {}
  ~ExprInNot() {
    delete _exprData;
    delete _exprArray;
  }

  ExprType GetType() override { return ExprType::EXPR_IN_OR_NOT; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *pdv = _exprData->Calc(vdParas, vdRow);
    if (pdv == nullptr)
      return TriBool::Error;

    bool b = _exprArray->Exist(pdv);
    pdv->DecRef();
    return (_bIn ^ b) ? TriBool::True : TriBool::False;
  }

protected:
  ExprData *_exprData;
  ExprArray *_exprArray;
  bool _bIn;
};

class ExprIsNullNot : public ExprLogic {
public:
  ExprIsNullNot(ExprData *child, bool bNull) : _child(child), _bNull(bNull) {}
  ~ExprIsNullNot() { delete _child; }

  ExprType GetType() override { return ExprType::EXPR_IS_NULL_NOT; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *pdv = _child->Calc(vdParas, vdRow);
    if (pdv == nullptr)
      return TriBool::Error;

    bool b = pdv->IsNull();
    pdv->DecRef();
    return (_bNull ^ b) ? TriBool::True : TriBool::False;
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

  ExprType GetType() override { return ExprType::EXPR_BETWEEN; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    IDataValue *pdv = _child->Calc(vdParas, vdRow);
    IDataValue *left = _exprLeft->Calc(vdParas, vdRow);
    IDataValue *right = _exprRight->Calc(vdParas, vdRow);
    if (pdv == nullptr || left == nullptr || right == nullptr) {
      if (pdv != nullptr)
        pdv->DecRef();
      if (left != nullptr)
        left->DecRef();
      if (right != nullptr)
        right->DecRef();
      return TriBool::Error;
    }

    bool b = (*pdv >= *left && *pdv <= *right);

    pdv->DecRef();
    left->DecRef();
    right->DecRef();
    return b ? TriBool::True : TriBool::False;
  }

public:
  ExprData *_child;
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprLike : public ExprLogic {
public:
  ExprLike(ExprData *exprData, IDataValue *dvPatten, bool blike)
      : _child(exprData), _dvPatten(dvPatten), _bLike(blike) {
    assert(dvPatten->IsStringType());
  }
  ~ExprLike() {
    delete _child;
    if (_dvPatten != nullptr)
      _dvPatten->DecRef();
  }

  ExprType GetType() override { return ExprType::EXPR_LIKE; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    // TO DO
    return TriBool::True;
  }

public:
  ExprData *_child;
  IDataValue *_dvPatten;
  bool _bLike;
};

class ExprNot : public ExprLogic {
public:
  ExprNot(ExprLogic *child) : _child(child) {}
  ~ExprNot() { delete _child; }

  ExprType GetType() override { return ExprType::EXPR_NOT; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    TriBool tb = _child->Calc(vdParas, vdRow);
    if (tb == TriBool::Error)
      return tb;
    return tb == TriBool::True ? TriBool::False : TriBool::True;
  }

public:
  ExprLogic *_child;
};

class ExprAnd : public ExprLogic {
public:
  ExprAnd() {}
  ~ExprAnd() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  ExprType GetType() override { return ExprType::EXPR_AND; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    for (ExprLogic *expr : _vctChild) {
      TriBool tb = expr->Calc(vdParas, vdRow);
      if (tb == TriBool::Error || tb == TriBool::False)
        return tb;
    }

    return TriBool::True;
  }

public:
  MVector<ExprLogic *> _vctChild;
};

class ExprOr : public ExprLogic {
public:
  ExprOr() {}
  ~ExprOr() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  ExprType GetType() override { return ExprType::EXPR_OR; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    for (ExprLogic *expr : _vctChild) {
      TriBool tb = expr->Calc(vdParas, vdRow);

      if (tb == TriBool::Error || tb == TriBool::True)
        return tb;
    }
    return TriBool::False;
  }

public:
  MVector<ExprLogic *> _vctChild;
};

} // namespace storage
