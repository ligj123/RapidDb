#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataValueFactory.h"
#include "../dataType/IDataValue.h"
#include "../table/Table.h"
#include "../utils/ErrorMsg.h"
#include "BaseExpr.h"
#include "ExprData.h"

#include <unordered_set>

using namespace std;
namespace storage {

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

    if (!left->AbleCompare(*right)) {
      left->DecRef();
      right->DecRef();
      return TriBool::Error;
    }

    bool b = false;
    switch (_compType) {
    case CompType::EQ:
      b = (*left == *right);
      break;
    case CompType::GT:
      b = (*left > *right);
      break;
    case CompType::GE:
      b = (*left >= *right);
      break;
    case CompType::LT:
      b = (*left < *right);
      break;
    case CompType::LE:
      b = (*left <= *right);
      break;
    case CompType::NE:
      b = (*left != *right);
      break;
    default:
      abort();
    }

    left->DecRef();
    right->DecRef();
    return b ? TriBool::True : TriBool::False;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_COMP) {
      vctElem.push_back(this);
    }

    _exprLeft->CollectElem(type, vctElem);
    _exprRight->CollectElem(type, vctElem);
  }

  void Reverse() {
    ExprData *tmp = _exprLeft;
    _exprLeft = _exprRight;
    _exprRight = tmp;
    switch (_compType) {
    case CompType::EQ:
      break;
    case CompType::GT:
      _compType = CompType::LT;
      break;
    case CompType::GE:
      _compType = CompType::LE;
      break;
    case CompType::LT:
      _compType = CompType::GT;
      break;
    case CompType::LE:
      _compType = CompType::GE;
      break;
    case CompType::NE:
      break;
    default:
      abort();
    }
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    if (_exprRight->GetType() == ExprType::EXPR_FIELD &&
        _exprLeft->IsConstValue()) {
      Reverse();
    }

    if (_exprLeft->GetType() != ExprType::EXPR_FIELD ||
        !_exprRight->IsConstValue()) {
      return TriBool::False;
    }

    const MHashMap<uint32_t, uint32_t> &map = table->GetIndexFirstFieldMap();
    ExprField *field = dynamic_cast<ExprField *>(_exprLeft);
    auto iter = map.find(field->_rowPos);
    if (iter == map.end()) {
      return TriBool::False;
    }

    idxPos = iter->second;

    return TriBool::True;
  }

  int CombinedIndexCondition(MVector<IndexColumn> &vctCol) override {
    assert(_exprLeft->GetType() == ExprType::EXPR_FIELD);
    if (_compType != CompType::EQ) {
      return -1;
    }

    ExprField *field = dynamic_cast<ExprField *>(_exprLeft);
    for (size_t i = 1; i < vctCol.size(); i++) {
      IndexColumn &icol = vctCol[i];
      if (icol.colPos == field->_rowPos) {
        return i;
      }
    }
    return -1;
  }

public:
  CompType _compType;
  ExprData *_exprLeft;
  ExprData *_exprRight;
};

class ExprInNot : public ExprLogic {
public:
  ExprInNot(ExprData *exprData, ExprArray *exprArray, bool bIn = true)
      : _exprData(exprData), _exprArray(exprArray), _bIn(bIn) {}
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
    return (_bIn ^ b) ? TriBool::False : TriBool::True;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_IN_OR_NOT) {
      vctElem.push_back(this);
    }

    _exprData->CollectElem(type, vctElem);
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    if (!_bIn || _exprData->GetType() != ExprType::EXPR_FIELD) {
      return TriBool::False;
    }

    const MHashMap<uint32_t, uint32_t> &map = table->GetIndexFirstFieldMap();
    ExprField *field = dynamic_cast<ExprField *>(_exprData);
    auto iter = map.find(field->_rowPos);
    if (iter == map.end()) {
      return TriBool::False;
    }

    idxPos = iter->second;
    return TriBool::True;
  }

  int CombinedIndexCondition(MVector<IndexColumn> &vctCol) override {
    assert(_exprData->GetType() == ExprType::EXPR_FIELD);

    ExprField *field = dynamic_cast<ExprField *>(_exprData);
    for (size_t i = 1; i < vctCol.size(); i++) {
      IndexColumn &icol = vctCol[i];
      if (icol.colPos == field->_rowPos) {
        return i;
      }
    }

    return -1;
  }

public:
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
    return (_bNull ^ b) ? TriBool::False : TriBool::True;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_IS_NULL_NOT) {
      vctElem.push_back(this);
    }

    _child->CollectElem(type, vctElem);
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    return TriBool::False;
  }

public:
  ExprData *_child;
  // The value of ExprData equal null or not
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

    if (!pdv->AbleCompare(*left) || !pdv->AbleCompare(*right)) {
      pdv->DecRef();
      left->DecRef();
      right->DecRef();
      return TriBool::Error;
    }

    bool b = *pdv >= *left && *pdv <= *right;
    pdv->DecRef();
    left->DecRef();
    right->DecRef();
    return b ? TriBool::True : TriBool::False;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_BETWEEN) {
      vctElem.push_back(this);
    }

    _child->CollectElem(type, vctElem);
    _exprLeft->CollectElem(type, vctElem);
    _exprRight->CollectElem(type, vctElem);
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    if (_child->GetType() != ExprType::EXPR_FIELD ||
        !_exprLeft->IsConstValue() || !_exprRight->IsConstValue()) {
      return TriBool::False;
    }

    const MHashMap<uint32_t, uint32_t> &map = table->GetIndexFirstFieldMap();
    ExprField *field = dynamic_cast<ExprField *>(_exprLeft);
    auto iter = map.find(field->_rowPos);
    if (iter == map.end()) {
      return TriBool::False;
    }

    idxPos = iter->second;

    return TriBool::True;
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
    return TriBool::False;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_LIKE) {
      vctElem.push_back(this);
    }

    _child->CollectElem(type, vctElem);
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    return TriBool::False;
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

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_NOT) {
      vctElem.push_back(this);
    }

    _child->CollectElem(type, vctElem);
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    return TriBool::False;
  }

public:
  ExprLogic *_child;
};

class ExprAnd : public ExprLogic {
public:
  ExprType GetType() override { return ExprType::EXPR_AND; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    for (ExprLogic *expr : _vctChild) {
      TriBool tb = expr->Calc(vdParas, vdRow);
      if (tb == TriBool::Error || tb == TriBool::False)
        return tb;
    }

    return TriBool::True;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_AND) {
      vctElem.push_back(this);
    }
    for (ExprLogic *child : _vctChild) {
      child->CollectElem(type, vctElem);
    }
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    struct IdxLogic {
      MVector<ExprLogic *>::iterator iter;
      int idxPos;
    };

    MVector<IdxLogic> vctIL;
    vctIL.reserve(_vctChild.size());

    for (auto iter = _vctChild.begin(); iter != _vctChild.end(); iter++) {
      ExprLogic *logic = *iter;
      if (logic->GetType() == ExprType::EXPR_AND) {
        continue;
      }

      IdxLogic il;
      TriBool b = logic->PickIndexCondition(table, il.idxPos, nullptr);
      if (b == TriBool::Error) {
        return TriBool::Error;
      } else if (b == TriBool::False) {
        continue;
      }

      if (vctIL.size() > 0) {
        if (vctIL[0].idxPos < il.idxPos) {
          continue;
        } else if (vctIL[0].idxPos > il.idxPos) {
          vctIL.clear();
        }
      }

      if (logic->GetType() == ExprType::EXPR_OR) {
        if (vctIL.size() == 0) {
          il.iter = iter;
          vctIL.push_back(il);
        }
        break;
      }

      il.iter = iter;
      vctIL.push_back(il);
    }

    if (vctIL.size() == 0) {
      return TriBool::False;
    }

    for (auto iter = vctIL.rbegin(); iter != vctIL.rend(); iter++) {
      vctExpr->push_back(*(iter->iter));
      _vctChild.erase(iter->iter);
    }

    idxPos = vctIL.begin()->idxPos;
    return TriBool::True;
  }

public:
  MVectorPtr<ExprLogic *> _vctChild;
};

class ExprOr : public ExprLogic {
public:
  ExprType GetType() override { return ExprType::EXPR_OR; }
  TriBool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow) override {
    for (ExprLogic *expr : _vctChild) {
      TriBool tb = expr->Calc(vdParas, vdRow);

      if (tb == TriBool::Error || tb == TriBool::True)
        return tb;
    }

    return TriBool::False;
  }

  void CollectElem(ExprType type, MVector<ExprElem *> &vctElem) override {
    if (type == ExprType::EXPR_OR) {
      vctElem.push_back(this);
    }
    for (ExprLogic *child : _vctChild) {
      child->CollectElem(type, vctElem);
    }
  }

  TriBool PickIndexCondition(PhysTable *table, int &idxPos,
                             MVectorPtr<ExprLogic *> *vctExpr) override {
    idxPos = -1;
    for (auto logic : _vctChild) {
      int ipos = -1;
      TriBool b = logic->PickIndexCondition(table, ipos, nullptr);
      if (b != TriBool::True) {
        return b;
      }

      if (idxPos == -1) {
        idxPos = ipos;
      } else if (idxPos != ipos) {
        return TriBool::False;
      }
    }

    return TriBool::True;
  }

public:
  MVectorPtr<ExprLogic *> _vctChild;
};

} // namespace storage
