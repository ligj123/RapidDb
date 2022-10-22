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

class ExprIn : public ExprLogic {
public:
  ExprIn(ExprColumn *exprColumn, ExprArray *exprArray)
      : _exprColumn(exprColumn), _exprArray(exprArray) {}
  ~ExprIn() {
    delete _exprColumn;
    delete _exprArray;
  }

  ExprType GetType() { return ExprType::EXPR_IN; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _exprColumn->Calc(vdPara, vdRow);
    bool b = _exprArray->Exist(pdv);
    if (!pdv->IsReuse())
      return false;

    return true;
  }

protected:
  ExprColumn *_exprColumn;
  ExprArray *_exprArray;
};

class ExprLike : public ExprLogic {
public:
  ExprLike(ExprColumn *exprColumn, ExprConst *exprPatten)
      : _exprColumn(exprColumn), _exprPatten(exprPatten) {
    // IDataValue *patt = exprPatten->GetValue();
    // if (!patt->IsStringType()) {
    //   throw ErrorMsg(DT_UNSUPPORT_OPER,
    //                  {"LIKE right", StrOfDataType(patt->GetDataType())});
    // }

    // if (patt->GetDataType() == DataType::FIXCHAR)
    //   patten = *(DataValueFixChar *)patt;
    // else
    //   patten = *(DataValueVarChar *)patt;
    // bLper = (patten[0] == '%');
    // bRper = (patten[patten.size() - 1] == '%');

    // if (bLper && bRper) {
    //   if (patten.size() == 1)
    //     patten = "";
    //   else
    //     patten = patten.substr(1, patten.size() - 2);
    // } else if (bLper) {
    //   patten = patten.substr(1);
    // } else if (bRper) {
    //   patten = patten.substr(0, patten.size() - 1);
    // }
  }
  ~ExprLike() {
    delete _exprColumn;
    delete _exprPatten;
  }

  ExprType GetType() { return ExprType::EXPR_LIKE; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    // IDataValue *left = _exprColumn->Calc(vdPara, vdRow);

    // if (!left->IsStringType()) {
    //   throw ErrorMsg(DT_UNSUPPORT_OPER,
    //                  {"LIKE left", StrOfDataType(left->GetDataType())});
    // }

    // MString lStr;
    // if (left->GetDataType() == DataType::FIXCHAR)
    //   lStr = *(DataValueFixChar *)left;
    // else
    //   lStr = *(DataValueVarChar *)left;
    // if (!left->IsReuse())
    //   delete left;

    // if (lStr.size() < patten.size())
    //   return false;
    // if (bLper && bRper)
    //   return (lStr.find(patten) != MString::npos);
    // else if (bLper) {
    //   return (lStr.substr(lStr.size() - patten.size()) == patten);
    // } else if (bRper) {
    //   return (lStr.substr(0, patten.size()) == patten);
    // } else {
    //   return (lStr == patten);
    // }
  }

protected:
  ExprColumn *_exprColumn;
  ExprConst *_exprPatten;
  MString _strPattern;
  bool _bLPer;
  bool _bRPer;
};

class ExprNot : public ExprLogic {
public:
  ExprNot(ExprLogic *child) : _child(child) {}
  ~ExprNot() { delete _child; }

  ExprType GetType() { return ExprType::EXPR_NOT; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return !_child->Calc(vdPara, vdRow);
  }

protected:
  ExprLogic *_child;
};

class ExprIsNull : public ExprLogic {
public:
  ExprIsNull(ExprData *child) : _child(child) {}
  ~ExprIsNull() { delete _child; }

  ExprType GetType() { return ExprType::EXPR_ISNULL; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _child->Calc(vdPara, vdRow);
    bool b = pdv->IsNull();
    if (!pdv->IsReuse())
      delete pdv;
    return b;
  }

protected:
  ExprData *_child;
};

class ExprComp : public ExprLogic {
public:
  ExprComp(CompType type, ExprData *l, ExprData *r)
      : _compType(type), _exprLeft(l), _exprRight(r) {}

  ExprType GetType() { return ExprType::EXPR_COMP; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->Calc(vdPara, vdRow);
    IDataValue *right = _exprRight->Calc(vdPara, vdRow);
    switch (_compType) {
    case CompType::EQ:
      return (*left == *right);
    case CompType::GT:
      return (*left > *right);
    case CompType::GE:
      return (*left >= *right);
    case CompType::LT:
      return (*left < *right);
    case CompType::LE:
      return (*left <= *right);
    case CompType::NE:
      return (*left != *right);
    default:
      abort();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return false;
  }

protected:
  CompType _compType;
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprAnd : public ExprLogic {
public:
  ExprAnd(MVector<ExprLogic *>::Type &vctChild) { _vctChild.swap(vctChild); }
  ~ExprAnd() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  ExprType GetType() { return ExprType::EXPR_AND; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    bool b = true;
    for (ExprLogic *expr : _vctChild) {
      b = b && expr->Calc(vdPara, vdRow);
      if (!b)
        break;
    }
    return b;
  }

protected:
  MVector<ExprLogic *>::Type _vctChild;
};

class ExprOr : public ExprLogic {
public:
  ExprOr(MVector<ExprLogic *>::Type vctChild) { _vctChild.swap(vctChild); }
  ~ExprOr() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  ExprType GetType() { return ExprType::EXPR_OR; }
  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    bool b = false;
    for (ExprLogic *expr : _vctChild) {
      b = b || expr->Calc(vdPara, vdRow);
      if (b)
        break;
    }
    return b;
  }

protected:
  MVector<ExprLogic *>::Type _vctChild;
};

class ExprCondition : public ExprLogic {
public:
  ExprCondition(ExprLogic *child, ExprLogic *indexExpr, MString indexName)
      : _child(child), _indexExpr(indexExpr), _indexName(indexName) {}
  ~ExprCondition() { delete _child; }

  ExprType GetType() { return ExprType::EXPR_CONDITION; }
  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_child == nullptr)
      return true;

    return _child->Calc(vdPara, vdRow);
  }

  BaseExpr *GetIndexExpr() { return _indexExpr; }
  MString GetIndexName() { return _indexName; }

protected:
  // Other query conditions except index condition, maybe null without other
  // query conditions.
  ExprLogic *_child;
  // If the primary or secondary index can be used to query, move the query
  // conditions to here. Only one index can be used.
  ExprLogic *_indexExpr;
  // Index name, only used for secondary index. If primary index, it will be
  // empty.
  MString _indexName;
};

class ExprOn : public ExprLogic {
public:
  ExprOn(MVector<ExprComp *>::Type vctChild) { _vctChild.swap(vctChild); }
  ~ExprOn() {
    for (auto c : _vctChild)
      delete c;
  }

  ExprType GetType() { return ExprType::EXPR_ON; }
  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    bool b = true;
    for (auto expr : _vctChild)
      b = b && expr->Calc(vdPara, vdRow);
    return b;
  }

protected:
  MVector<ExprComp *>::Type _vctChild;
};
} // namespace storage
