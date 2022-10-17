#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include <unordered_set>

using namespace std;
namespace storage {
enum class ExprType {
  EXPR_BASE,
  // const type
  EXPR_ARRAY,
  EXPR_CONST,

  // return data value
  EXPR_PARAMETER,
  EXPR_COLUMN,
  EXPR_ADD,
  EXPR_SUB,
  EXPR_MUL,
  EXPR_DIV,
  EXPR_MINUS,

  // Aggr
  EXPR_COUNT,
  EXPR_SUM,
  EXPR_MAX,
  EXPR_MIN,
  EXPR_AVG,

  // return bool
  EXPR_AND,
  EXPR_OR,
  EXPR_COMP,
  EXPR_IN,
  EXPR_LIKE,
  EXPR_NOT,
  EXPR_ISNULL,
  EXPR_CONDITION,
  EXPR_ON,

  // Input or oupt value
  EXPR_VALUE_ARRAY_IN,
  EXPR_VALUE_IN,
  EXPR_VALUE_ARRAY_OUT,
  EXPR_VALUE_OUT,

  // Select
  EXPR_SELECT,
  EXPR_JOIN,
  EXPR_GROUP_BY,

  EXPR_INSERT,
  EXPR_UPDATE,
  EXPR_DELETE,

  // Internal function fro SQL
  EXPR_FUNCTION
};

/**
 * @brief base class for all expression
 */
class BaseExpr {
public:
  virtual ~BaseExpr() {}
  ExprType GetType() { return ExprType::EXPR_BASE; }

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
};

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

/**@brief Base class for all aggressive expression*/
class ExprAggr : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  virtual void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) = 0;
};

// The base class for Columns of input or output
class ExprColumn : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  virtual void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) = 0;

protected:
  int _index; // The index in table's columns array
  DataType _dataType;
};

/**
 * @brief To save array values, to be used in SQL operator IN.
 */
class ExprArray : public BaseExpr {
public:
  ExprArray(VectorDataValue &vctVal) {
    _setVal.insert(vctVal.begin(), vctVal.end());
    vctVal.clear();
  }
  ~ExprArray() {
    for (IDataValue *pdv : _setVal) {
      delete pdv;
    }
  }

  ExprType GetType() { return ExprType::EXPR_ARRAY; }
  bool Exist(IDataValue *pdv) { return (_setVal.find(pdv) != _setVal.end()); }

protected:
  unordered_set<IDataValue *, DataValueHash, DataValueEqual,
                Mallocator<IDataValue *>>
      _setVal;
};

} // namespace storage
