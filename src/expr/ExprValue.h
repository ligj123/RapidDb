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
 * @brief The input fields for insert or update.
 */
class ExprValueIn : public ExprValue {
public:
  // index: the order from zero for this column in expression value array
  // dvConst: The const parameter
  ExprValueIn(int index, IDataValue *dvConst)
      : _index(index), valConst(true), _child(nullptr), _dvConst(dvConst) {}
  // child: For normal parameter, it will tell how to get parameter.
  ExprValueIn(int index, ExprData *child)
      : _index(index), valConst(false), _child(child), _dvConst(nullptr) {}
  ~ExprValueIn() {
    delete _child;
    delete _dvConst;
  }

  ExprType GetType() { return ExprType::EXPR_VALUE_IN; }
  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (valConst) {
      vdRow[_index]->Copy(*_dvConst, false);
    } else {
      IDataValue *pDv = _child->Calc(vdPara, vdRow);
      vdRow[_index]->Copy(*pDv, !pDv->IsReuse());
      if (!pDv->IsReuse())
        delete pDv;
    }
  }

  int GetIndex() { return _index; }

protected:
  int _index; // The index in table's colu
  mns array
  // true: the parameter is const value and saved in variable _dvConst.
  // false: The parameter's value come from outside input, get by _child
  bool valConst;
  // For no const type, to tell how to get parameter from array.
  ExprData *_child;
  // For const type, to save the const parameter value.
  IDataValue *_dvConst;
};

/**
 *@brief To calc the array value for insert or update.
 */
class ExprValueArrayIn : public ExprValue {
public:
  ExprValueArrayIn(MVector<ExprValueIn *>::Type &vctVal) {
    _vctVal.swap(vctVal);
  }

  ExprType GetType() { return ExprType::EXPR_VALUE_ARRAY_IN; }
  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    for (auto iter : _vctVal) {
      iter->Calc(vdPara, vdRow);
    }
  }

protected:
  MVector<ExprValueIn *>::Type _vctVal;
};

/**
 * @brief To calc a column value for select.
 */
class ExprValueOut : public ExprValue {
public:
  // index: the order from zero for this column in value array
  // child: To tell how to get parameter
  ExprValueOut(int index, ExprData *child) : _index(index), _child(child) {}
  ~ExprValueOut() { delete _child; }

  ExprType GetType() { return ExprType::EXPR_VALUE_OUT; }
  void Calc(VectorDataValue &vdSour, VectorDataValue &vdDest) {
    vdDest[_index] = _child->Calc(vdSour, vdDest);
  }

  int GetIndex() { return _index; }

protected:
  int _index;       // The index in value array
  ExprData *_child; // To tell how to get parameter.
};
/**
 *@brief For insert or update inpout values or select output values.
 */
class ExprValueArrayOut : public ExprValue {
public:
  ExprValueArrayOut(MVector<ExprValueOut *>::Type &vctVal) {
    _vctVal.swap(vctVal);
  }

  ExprType GetType() { return ExprType::EXPR_VALUE_ARRAY_OUT; }
  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    for (auto iter : _vctVal) {
      iter->Calc(vdPara, vdRow);
    }
  }

protected:
  MVector<ExprValueOut *>::Type _vctVal;
};
} // namespace storage
