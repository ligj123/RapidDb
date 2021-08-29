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
#include <vector>

using namespace std;
namespace storage {
class ExprAnd : public ExprLogic {
public:
  ExprAnd(vector<ExprLogic *> &vctChild) : ExprLogic(ExprType::EXPR_AND) {
    _vctChild.swap(vctChild);
  }
  ~ExprAnd() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    bool b = true;
    for (ExprLogic *expr : _vctChild) {
      b = b && expr->Calc(vdPara, vdRow);
    }
    return b;
  }

protected:
  vector<ExprLogic *> _vctChild;
};

class ExprOr : public ExprLogic {
public:
  ExprOr(vector<ExprLogic *> vctChild) : ExprLogic(ExprType::EXPR_OR) {
    _vctChild.swap(vctChild);
  }
  ~ExprOr() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    bool b = false;
    for (ExprLogic *expr : _vctChild) {
      b = b || expr->Calc(vdPara, vdRow);
    }
    return b;
  }

protected:
  vector<ExprLogic *> _vctChild;
};

class ExprIn : public ExprLogic {
public:
  ExprIn(ExprColumn *exprColumn, ExprArray *exprArray)
      : ExprLogic(ExprType::EXPR_IN), _exprColumn(exprColumn),
        _exprArray(exprArray) {}
  ~ExprIn() {
    delete _exprColumn;
    delete _exprArray;
  }

  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _exprColumn->Calc(vdPara, vdRow);
    bool b = _exprArray->Exist(pdv);
    if (!pdv->IsReuse())
      return false;
  }

protected:
  ExprColumn *_exprColumn;
  ExprArray *_exprArray;
};

class ExprLike : public ExprLogic {
public:
  ExprLike(ExprColumn *exprColumn, ExprConst *exprPatten)
      : ExprLogic(ExprType::EXPR_LIKE), _exprColumn(exprColumn),
        _exprPatten(exprPatten) {
    IDataValue *patt = exprPatten->GetVal();
    if (!patt->IsStringType()) {
      throw ErrorMsg(DT_UNSUPPORT_OPER,
                     {"LIKE right", StrOfDataType(patt->GetDataType())});
    }

    if (patt->GetDataType() == DataType::FIXCHAR)
      patten = *(DataValueFixChar *)patt;
    else
      patten = *(DataValueVarChar *)patt;
    bLper = (patten[0] == '%');
    bRper = (patten[patten.size() - 1] == '%');

    if (bLper && bRper) {
      if (patten.size() == 1)
        patten = "";
      else
        patten = patten.substr(1, patten.size() - 2);
    } else if (bLper) {
      patten = patten.substr(1);
    } else if (bRper) {
      patten = patten.substr(0, patten.size() - 1);
    }
  }
  ~ExprLike() {
    delete _exprColumn;
    delete _exprPatten;
  }

  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprColumn->Calc(vdPara, vdRow);

    if (!left->IsStringType()) {
      throw ErrorMsg(DT_UNSUPPORT_OPER,
                     {"LIKE left", StrOfDataType(left->GetDataType())});
    }

    string lStr;
    if (left->GetDataType() == DataType::FIXCHAR)
      lStr = *(DataValueFixChar *)left;
    else
      lStr = *(DataValueVarChar *)left;
    if (!left->IsReuse())
      delete left;

    if (lStr.size() < patten.size())
      return false;
    if (bLper && bRper)
      return (lStr.find(patten) != string::npos);
    else if (bLper) {
      return (lStr.substr(lStr.size() - patten.size()) == patten);
    } else if (bRper) {
      return (lStr.substr(0, patten.size()) == patten);
    } else {
      return (lStr == patten);
    }
  }

protected:
  ExprColumn *_exprColumn;
  ExprConst *_exprPatten;
  string patten;
  bool bLper;
  bool bRper;
};

class ExprNot : public ExprLogic {
public:
  ExprNot(ExprLogic *child) : ExprLogic(ExprType::EXPR_NOT), _child(child) {}
  ~ExprNot() { delete _child; }

  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return !_child->Calc(vdPara, vdRow);
  }

protected:
  ExprLogic *_child;
};

class ExprIsNull : public ExprLogic {
public:
  ExprIsNull(ExprData *child)
      : ExprLogic(ExprType::EXPR_ISNULL), _child(child) {}
  ~ExprIsNull() { delete _child; }

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
      : ExprLogic(ExprType::EXPR_COMP), _compType(type), _exprLeft(l),
        _exprRight(r) {}
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

class ExprWhere : public ExprLogic {
public:
  ExprWhere(ExprLogic *child, ExprLogic *indexExpr, string indexName)
      : ExprLogic(ExprType::EXPR_WHERE), _child(child), _indexExpr(indexExpr),
        _indexName(indexName) {}
  ~ExprWhere() { delete _child; }
  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_child == nullptr)
      return true;

    return _child->Calc(vdPara, vdRow);
  }

  BaseExpr *GetIndexExpr() { return _indexExpr; }
  string GetIndexName() { return _indexName; }

protected:
  // Other query conditions except index condition, maybe null without other
  // query conditions.
  ExprLogic *_child;
  // If the primary or secondary index can be used to query, move the query
  // conditions to here. Only one index can be used.
  ExprLogic *_indexExpr;
  // Index name, only used for secondary index. If primary index, it will be
  // empty.
  string _indexName;
};

class ExprOn : public ExprLogic {
public:
  ExprOn(vector<ExprComp *> vctChild) : ExprLogic(ExprType::EXPR_ON) {
    _vctChild.swap(vctChild);
  }
  ~ExprOn() {
    for (auto c : _vctChild)
      delete c;
  }
  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    bool b = true;
    for (auto expr : _vctChild)
      b = b && expr->Calc(vdPara, vdRow);
    return b;
  }

protected:
  vector<ExprComp *> _vctChild;
};
} // namespace storage
