#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include <unordered_set>
#include <vector>

using namespace std;
namespace storage {
enum class CompType { EQ, GT, GE, LT, LE, NE };

enum class ExprType {
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
  EXPR_WHERE,
  Expr_ON,

  // Input or oupt value
  EXPR_VALUE_ARRAY_IN,
  EXPR_VALUE_IN,
  EXPR_VALUE_ARRAY_OUT,
  EXPR_VALUE_OUT,

  // Select
  EXPR_ORDER_BY,
  EXPR_SELECT,
  Expr_INNER_JOIN,
  EXPR_LEFT_JOIN,
  EXPR_RIGHT_JOIN,
  EXPR_OUTTER_JOIN,
  EXPR_GROUP_BY,
  //

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
  BaseExpr(ExprType t) : _exprType(t) {}
  virtual ~BaseExpr() {}
  ExprType GetType() { return _exprType; }

protected:
  // Every expression must set one of type in ExprType.
  ExprType _exprType;
};

/**
 * @brief Base class for all expression to calc and return data value.
 */
class ExprData : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  /**Returned DataValue need user to release self.*/
  virtual IDataValue *Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) = 0;
};

class ExprLogic : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  // The expression below EXPR_SPLIT to call this functionto calc and return
  // bool value.
  virtual bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) = 0;
};

class ExprAggr : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  virtual void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) = 0;
};

class ExprConst : public BaseExpr {
public:
  ExprConst(IDataValue *val) : BaseExpr(ExprType::EXPR_CONST), _val(val) {}
  ~ExprConst() { delete _val; }

  IDataValue *GetVal() { return _val; }

protected:
  IDataValue *_val;
};

// The base class for values of input or output
class ExprValue : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  virtual void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) = 0;
};
/**
 * @brief To save array values, used to follow expression IN.
 */
class ExprArray : public BaseExpr {
public:
  ExprArray(VectorDataValue &vctVal) : BaseExpr(ExprType::EXPR_ARRAY) {
    _setVal.insert(vctVal.begin(), vctVal.end());
    vctVal.clear();
  }
  ~ExprArray() {
    for (IDataValue *pdv : _setVal) {
      delete pdv;
    }
  }

  bool Exist(IDataValue *pdv) { return (_setVal.find(pdv) != _setVal.end()); }

protected:
  unordered_set<IDataValue *, DataValueHash, DataValueEqual> _setVal;
};

} // namespace storage
