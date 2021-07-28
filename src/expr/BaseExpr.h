#pragma once
#include "../dataType/IDataValue.h"
#include <vector>

using namespace std;
namespace storage {
enum class ExprType {
  EXPR_COLUMNS,
  EXPR_WHERE,
  EXPR_ON,
  EXPR_VALUE,
  EXPR_PARA,
  EXPR_COLUMN,
  EXPR_FUNCTION,
  EXPR_OPERATOR,
  EXPR_SELECT,
  EXPR_ARRAY,
  EXPR_CASE,
  EXPR_ADD,
  EXPR_SUB,
  EXPR_MUL,
  EXPR_DIV,
  EXPR_AND,
  EXPR_OR,
  EXPR_IN,
  EXPR_LIKE,
  EXPR_NOT,
  EXPR_ISNULL,
  EXPR_EXIST,
  EXPR_MINUS
};

class BaseExpr {
public:
  ~BaseExpr() {}
  virtual IDataValue *op(VectorDataValue &vctDv) = 0;
  ExprType GetType() { return _exprType; }

protected:
  ExprType _exprType;
};

class ValueExpr : BaseExpr {
public:
  ValueExpr(int vPos) : _valPos(vPos) {}
  IDataValue *op(VectorDataValue &vctDv) {}

protected:
  int _valPos;
};
class ParameterExpr : BaseExpr {};

class ColumnExpr : BaseExpr {};

class FunctionExpr : BaseExpr {};

class OperatorExpr : BaseExpr {};

class SelectExpr : BaseExpr {};

class ArrayExpr : BaseExpr {};

class CaseExpr : BaseExpr {};
} // namespace storage
