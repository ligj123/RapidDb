#pragma once
#include "../dataType/IDataValue.h"
#include <vector>

using namespace std;
namespace storage {
enum class Direction {
  DD_IN,  // Input the value to table, used for insert or update set.
  DD_OUT, // Read value from table,used for where clause.
};

enum class CompType { EQ, GT, GE, LT, LE, NE };

enum class ExprType {
  EXPR_SELECT,
  EXPR_INSERT,
  EXPR_UPDATE,
  EXPR_DELETE,
  EXPR_VALUE_ARRAY,
  EXPR_WHERE,
  EXPR_ON,
  EXPR_VALUE,
  EXPR_PARA,
  EXPR_COLUMN,
  EXPR_FUNCTION,
  EXPR_ARRAY,
  EXPR_CONST,
  EXPR_ADD,
  EXPR_SUB,
  EXPR_MUL,
  EXPR_DIV,
  EXPR_MINUS,
  // Below expression return bool type
  EXPR_AND,
  EXPR_OR,
  EXPR_COMP,
  EXPR_IN,
  EXPR_LIKE,
  EXPR_NOT,
  EXPR_ISNULL,
  EXPR_EXIST
};

/**
 * @brief base class for all expression
 */
class BaseExpr {
public:
  BaseExpr(ExprType t) : _exprType(t) {}
  ~BaseExpr() {}
  virtual IDataValue *calcVal(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    abort();
  }
  virtual bool calcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    abort();
  }

  ExprType GetType() { return _exprType; }
  bool IsRtBool() { return _exprType >= ExprType::EXPR_AND; }
  static void ClearLocalDataValue() { _vctDvLocal.RemoveAll(); }

protected:
  ExprType _exprType;
  static thread_local VectorDataValue _vctDvLocal;
};

/**
 * @brief Corresponding to a column in thw row.
 */
class ExprColumn : public BaseExpr {
public:
  ExprColumn(int rowPos) : BaseExpr(ExprType::EXPR_COLUMN), _rowPos(rowPos) {}
  IDataValue *calcVal(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return vdRow[_rowPos];
  }

protected:
  int _rowPos; // The position in the table.
};

/*
 * @brief Get Parameter from inputed parameter array.
 */
class ExprParameter : public BaseExpr {
public:
  ExprParameter(int paraPos)
      : BaseExpr(ExprType::EXPR_COLUMN), _paraPos(paraPos) {}
  IDataValue *calcVal(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return vdRow[_paraPos];
  }

protected:
  int _paraPos; // The position in parameter array.
};

/*
 * @brief The parent class for all function expression.
 */
class ExprFunc : public BaseExpr {
public:
protected:
  string _funcName;
};

class ExprConst : public BaseExpr {
public:
  ExprConst(IDataValue *val) : BaseExpr(ExprType::EXPR_CONST), _val(val) {}
  ~ExprConst() { delete _val; }
  IDataValue *calcVal(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return _val;
  }

protected:
  IDataValue *_val;
};
/**
 * @brief The input value for insert or update or output value for select.
 */
class ExprValue {
public:
  ExprValue(int index, Direction direct, int rowPos, bool nullable = false)
      : _index(index), _bSimple(true), _direct(direct), _child(nullptr),
        _rowPos(rowPos), _bNullable(nullable) {}
  ExprValue(int index, Direction direct, BaseExpr *child, bool nullable)
      : _index(index), _bSimple(true), _direct(direct), _child(child),
        _rowPos(-1), _bNullable(nullable) {}
  ~ExprValue() { delete _child; }
  void calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {}
  int GetIndex() { return _index; }

protected:
  int _index; // The index in value array
  Direction _direct;
  /* True, get or set value directionaly from input array or table rows
   * False, need to use function to calc before get the value
   */
  bool _bSimple;
  BaseExpr *_child; // Only used when _bSimple=false
  int _rowPos;     // Only used when _bSimple=true, the position in value array.
  bool _bNullable; // Only used for DD_IN, to judge if a value is null
};

/**
 *@brief For insert or update inpout values or select output values.
 */
class ExprValues {
public:
  ExprValues() {}
  ExprValues(vector<ExprValue *> &vctVal) { _vctVal.swap(vctVal); }

  void AddChild(ExprValue *child) { _vctVal.push_back(child); }
  void calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    for (auto iter : _vctVal) {
      iter->calc(vdPara, vdRow);
    }
  }

protected:
  vector<ExprValue *> _vctVal;
};

/**
 * @brief To save array values, used to follow expression IN.
 */
class ExprArray : public BaseExpr {
public:
  ExprArray(VectorDataValue &vctVal) : BaseExpr(ExprType::EXPR_ARRAY) {
    _vctVal.swap(vctVal);
  }

  const VectorDataValue &GetVals() { return _vctVal; }

protected:
  VectorDataValue _vctVal;
};

class ExprAdd : public BaseExpr {
public:
  ExprAdd(BaseExpr *left, BaseExpr *right)
      : BaseExpr(ExprType::EXPR_ADD), _exprLeft(left), _exprRight(right) {}
  ~ExprAdd() {
    delete _exprLeft;
    delete _exprRight;
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    IDataValue *left = _exprLeft->calcVal(vdPara, vdRow);
    IDataValue *right = _exprRight->calcVal(vdPara, vdRow);

    if (left->IsStringType() || right->IsStringType()) {
    }
    return nullptr;
  }

protected:
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};

class ExprSub : public BaseExpr {
public:
  ExprSub(BaseExpr *left, BaseExpr *right)
      : BaseExpr(ExprType::EXPR_SUB), _exprLeft(left), _exprRight(right) {}
  ~ExprSub() {
    delete _exprLeft;
    delete _exprRight;
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};

class ExprMul : public BaseExpr {
public:
  ExprMul(BaseExpr *left, BaseExpr *right)
      : BaseExpr(ExprType::EXPR_MUL), _exprLeft(left), _exprRight(right) {}
  ~ExprMul() {
    delete _exprLeft;
    delete _exprRight;
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};

class ExprDiv : public BaseExpr {
public:
  ExprDiv(BaseExpr *left, BaseExpr *right)
      : BaseExpr(ExprType::EXPR_DIV), _exprLeft(left), _exprRight(right) {}
  ~ExprDiv() {
    delete _exprLeft;
    delete _exprRight;
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};

class ExprAnd : public BaseExpr {
public:
  ExprAnd(vector<BaseExpr *> vctChild)
      : BaseExpr(ExprType::EXPR_AND), _vctChild(vctChild) {}
  ~ExprAnd() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  vector<BaseExpr *> _vctChild;
};

class ExprOr : public BaseExpr {
public:
  ExprOr(vector<BaseExpr *> vctChild)
      : BaseExpr(ExprType::EXPR_OR), _vctChild(vctChild) {}
  ~ExprOr() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  vector<BaseExpr *> _vctChild;
};

class ExprSelect;
class ExprIn : public BaseExpr {
public:
  ExprIn(ExprColumn *exprColumn, VectorDataValue vctVal)
      : BaseExpr(ExprType::EXPR_IN), bSel(false), _exprColumn(exprColumn),
        _vctVal(vctVal), _exprSel(nullptr) {}
  ExprIn(ExprColumn *exprColumn, ExprSelect *exprSel)
      : BaseExpr(ExprType::EXPR_IN), bSel(true), _exprColumn(exprColumn),
        _exprSel(exprSel) {}
  ~ExprIn() {
    delete _exprColumn;
    delete _exprSel;
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  bool bSel;
  ExprColumn *_exprColumn;
  VectorDataValue _vctVal;
  ExprSelect *_exprSel;
};

class ExprLike : public BaseExpr {
public:
  ExprLike(ExprColumn *exprColumn, ExprConst *exprConst)
      : BaseExpr(ExprType::EXPR_LIKE), _exprColumn(exprColumn),
        _exprConst(exprConst) {}
  ~ExprLike() {
    delete _exprColumn;
    delete _exprConst;
  }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  ExprColumn *_exprColumn;
  ExprConst *_exprConst;
};

class ExprNot : public BaseExpr {
public:
  ExprNot(BaseExpr *child) : BaseExpr(ExprType::EXPR_NOT), _child(child) {}
  ~ExprNot() { delete _child; }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  BaseExpr *_child;
};

class ExprIsNull : public BaseExpr {
public:
  ExprIsNull(BaseExpr *child)
      : BaseExpr(ExprType::EXPR_ISNULL), _child(child) {}
  ~ExprIsNull() { delete _child; }

  IDataValue *calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return nullptr;
  }

protected:
  BaseExpr *_child;
};

class ExprComp : public BaseExpr {
public:
  ExprComp(CompType type, BaseExpr *l, BaseExpr *r)
      : BaseExpr(ExprType::EXPR_COMP), _compType(type), _exprLeft(l),
        _exprRight(r) {}
  bool calcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    return false;
  }

protected:
  CompType _compType;
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};
} // namespace storage
