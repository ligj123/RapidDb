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
 * @brief The input value for insert or update.
 */
class ExprValueIn : public ExprValue {
public:
  // index: the order from zero for this column in value array
  // rowPos: For simple type, to use which element in parameter array
  ExprValueIn(int index, int rowPos)
      : ExprValue(ExprType::EXPR_VALUE_IN), _index(index), _bSimple(true),
        _child(nullptr), _rowPos(rowPos) {}
  ExprValueIn(int index, ExprData *child)
      : ExprValue(ExprType::EXPR_VALUE_IN), _index(index), _bSimple(false),
        _child(child), _rowPos(-1) {}
  ~ExprValueIn() { delete _child; }
  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_bSimple) {
      vdRow[_index]->Copy(*vdPara[_rowPos], false);
    } else {
      IDataValue *pDv = _child->Calc(vdPara, vdRow);
      vdRow[_index]->Copy(*pDv, !pDv->IsReuse());
      if (!pDv->IsReuse())
        delete pDv;
    }
  }

  int GetIndex() { return _index; }
  bool IsSimple() { return _bSimple; }

protected:
  int _index; // The index in value array
  /* True, get or set value directionaly from input array or table rows
   * False, need to use function to calc before get the value
   */
  bool _bSimple;
  ExprData *_child; // Only used when _bSimple=false
  int _rowPos;      // Only used when _bSimple=true, the position in parameter
};

/**
 *@brief To calc the array value for insert or update.
 */
class ExprValueArrayIn : public ExprValue {
public:
  ExprValueArrayIn(MVector<ExprValueIn *>::Type &vctVal)
      : ExprValue(ExprType::EXPR_VALUE_ARRAY_IN) {
    _vctVal.swap(vctVal);
  }

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
  // rowPos: For simple type, to use which element in source table
  ExprValueOut(int index, int rowPos)
      : ExprValue(ExprType::EXPR_VALUE_OUT), _index(index), _bSimple(true),
        _child(nullptr), _rowPos(rowPos) {}
  ExprValueOut(int index, ExprData *child)
      : ExprValue(ExprType::EXPR_VALUE_OUT), _index(index), _bSimple(false),
        _child(child), _rowPos(-1) {}
  ~ExprValueOut() { delete _child; }
  void Calc(VectorDataValue &vdSour, VectorDataValue &vdDest) {
    if (_bSimple) {
      vdDest[_index] = vdSour[_rowPos];
    } else {
      vdDest[_index] = _child->Calc(vdSour, vdDest);
    }
  }

  int GetIndex() { return _index; }

protected:
  int _index; // The index in value array
  /* True, get or set value directionaly from input array or table rows
   * False, need to use function to calc before get the value
   */
  bool _bSimple;
  ExprData *_child; // Only used when _bSimple=false
  int _rowPos;      // Only used when _bSimple=true, the position in parameter
};
/**
 *@brief For insert or update inpout values or select output values.
 */
class ExprValueArrayOut : public ExprValue {
public:
  ExprValueArrayOut(MVector<ExprValueOut *>::Type &vctVal)
      : ExprValue(ExprType::EXPR_VALUE_ARRAY_OUT) {
    _vctVal.swap(vctVal);
  }

  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    for (auto iter : _vctVal) {
      iter->Calc(vdPara, vdRow);
    }
  }

protected:
  MVector<ExprValueOut *>::Type _vctVal;
};
} // namespace storage
