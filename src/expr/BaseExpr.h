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
  EXPR_SELECT,
  EXPR_INSERT,
  EXPR_UPDATE,
  EXPR_DELETE,
  EXPR_VALUE_ARRAY,
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
  Expr_INNER_JOIN,
  EXPR_LEFT_JOIN,
  EXPR_RIGHT_JOIN,
  EXPR_OUTTER_JOIN,
  EXPR_SPLIT, // Split Line, below expression return bool type
  EXPR_AND,
  EXPR_OR,
  EXPR_COMP,
  EXPR_IN,
  EXPR_LIKE,
  EXPR_NOT,
  EXPR_ISNULL,
  EXPR_WHERE,
  Expr_ON
};

/**
 * @brief base class for all expression
 */
class BaseExpr {
public:
  BaseExpr(ExprType t) : _exprType(t) {}
  virtual ~BaseExpr() {}
  /**Returned DataValue need user to release self.*/
  virtual IDataValue *CalcVal(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    abort();
  }
  // The expression below EXPR_SPLIT to call this functionto calc and return
  // bool value.
  virtual bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    abort();
  }

  ExprType GetType() { return _exprType; }
  bool IsRtBool() { return _exprType >= ExprType::EXPR_SPLIT; }

protected:
  // Every expression must set one of type in ExprType.
  ExprType _exprType;
};

/**
 * @brief Corresponding to a column in the table or temp table.
 */
class ExprColumn : public BaseExpr {
public:
  ExprColumn(int rowPos) : BaseExpr(ExprType::EXPR_COLUMN), _rowPos(rowPos) {}
  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
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
  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
    return vdRow[_paraPos];
  }

protected:
  int _paraPos; // The position in parameter array.
};

class ExprConst : public BaseExpr {
public:
  ExprConst(IDataValue *val) : BaseExpr(ExprType::EXPR_CONST), _val(val) {}
  ~ExprConst() { delete _val; }
  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
    return _val;
  }
  IDataValue *GetVal() { return _val; }

protected:
  IDataValue *_val;
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

  bool Exist(IDataValue *pdv) { _setVal.find(pdv) != _setVal.end(); }

protected:
  unordered_set<IDataValue *, DataValueHash, DataValueEqual> _setVal;
};

/**
 * @brief The input value for insert or update.
 */
class ExprValueIn {
public:
  // index: the order from zero for this column in value array
  // rowPos: For simple type, to use which element in parameter array
  ExprValueIn(int index, int rowPos)
      : _index(index), _bSimple(true), _child(nullptr), _rowPos(rowPos) {}
  ExprValueIn(int index, BaseExpr *child)
      : _index(index), _bSimple(false), _child(child), _rowPos(-1) {}
  ~ExprValueIn() { delete _child; }
  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_bSimple) {
      vdRow[_index]->Copy(*vdPara[_rowPos], false);
    } else {
      IDataValue *pDv = _child->CalcVal(vdPara, vdRow);
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
  BaseExpr *_child; // Only used when _bSimple=false
  int _rowPos;      // Only used when _bSimple=true, the position in parameter
};

/**
 *@brief To calc the array value for insert or update.
 */
class ExprValueArrayIn {
public:
  ExprValueArrayIn() {}
  ExprValueArrayIn(vector<ExprValueIn *> &vctVal) { _vctVal.swap(vctVal); }

  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    for (auto iter : _vctVal) {
      iter->Calc(vdPara, vdRow);
    }
  }

protected:
  vector<ExprValueIn *> _vctVal;
};

/**
 * @brief To calc a column value for select.
 */
class ExprValueOut {
public:
  // index: the order from zero for this column in value array
  // rowPos: For simple type, to use which element in source table
  ExprValueOut(int index, int rowPos)
      : _index(index), _bSimple(true), _child(nullptr), _rowPos(rowPos) {}
  ExprValueOut(int index, BaseExpr *child)
      : _index(index), _bSimple(false), _child(child), _rowPos(-1) {}
  ~ExprValueOut() { delete _child; }
  void Calc(VectorDataValue &vdSour, VectorDataValue &vdDest) {
    if (_bSimple) {
      vdDest[_index] = vdSour[_rowPos];
    } else {
      vdDest[_index] = _child->CalcVal(vdSour, vdDest);
    }
  }

  int GetIndex() { return _index; }

protected:
  int _index; // The index in value array
  /* True, get or set value directionaly from input array or table rows
   * False, need to use function to calc before get the value
   */
  bool _bSimple;
  BaseExpr *_child; // Only used when _bSimple=false
  int _rowPos;      // Only used when _bSimple=true, the position in parameter
};
/**
 *@brief For insert or update inpout values or select output values.
 */
class ExprValueArrayOut {
public:
  ExprValueArrayOut(vector<ExprValueOut *> &vctVal) { _vctVal.swap(vctVal); }

  void Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    for (auto iter : _vctVal) {
      iter->Calc(vdPara, vdRow);
    }
  }

protected:
  vector<ExprValueOut *> _vctVal;
};

class ExprAdd : public BaseExpr {
public:
  ExprAdd(BaseExpr *left, BaseExpr *right)
      : BaseExpr(ExprType::EXPR_ADD), _exprLeft(left), _exprRight(right) {}
  ~ExprAdd() {
    delete _exprLeft;
    delete _exprRight;
  }

  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->CalcVal(vdPara, vdRow);
    IDataValue *right = _exprRight->CalcVal(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsStringType() || right->IsStringType()) {
      StrBuff sb(left->GetDataLength() * 2 + right->GetDataLength() * 2);
      left->ToString(sb);
      right->ToString(sb);
      rt = new DataValueVarChar(sb.GetBuff(), sb.GetBufLen() + 1,
                                sb.GetBufLen() + 1);
    } else if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() + right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() + right->GetDouble());
    } else {
      rt = left->Clone();
    }
    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
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

  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->CalcVal(vdPara, vdRow);
    IDataValue *right = _exprRight->CalcVal(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() - right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() - right->GetDouble());
    } else {
      rt = left->Clone();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
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

  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->CalcVal(vdPara, vdRow);
    IDataValue *right = _exprRight->CalcVal(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      rt = new DataValueLong(left->GetLong() * right->GetLong());
    } else if (left->IsDigital() && right->IsDigital()) {
      rt = new DataValueDouble(left->GetDouble() * right->GetDouble());
    } else {
      rt = left->Clone();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
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

  IDataValue *CalcVal(VectorDataValue &vdPara,
                      VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->CalcVal(vdPara, vdRow);
    IDataValue *right = _exprRight->CalcVal(vdPara, vdRow);
    IDataValue *rt = nullptr;

    if (left->IsAutoPrimaryKey() && right->IsAutoPrimaryKey()) {
      int64_t r = right->GetLong();
      if (r != 0)
        rt = new DataValueLong(left->GetLong() * r);
    } else if (left->IsDigital() && right->IsDigital()) {
      double r = right->GetDouble();
      if (r != 0)
        rt = new DataValueDouble(left->GetDouble() * r);
    }
    if (rt == nullptr) {
      rt = left->Clone();
    }

    if (!left->IsReuse())
      delete left;
    if (!right->IsReuse())
      delete right;
    return rt;
  }

protected:
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};

class ExprAnd : public BaseExpr {
public:
  ExprAnd(vector<BaseExpr *> vctChild) : BaseExpr(ExprType::EXPR_AND) {
    _vctChild.swap(vctChild);
  }
  ~ExprAnd() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    bool b = true;
    for (BaseExpr *expr : _vctChild) {
      b = b && expr->CalcBool(vdPara, vdRow);
    }
    return b;
  }

protected:
  vector<BaseExpr *> _vctChild;
};

class ExprOr : public BaseExpr {
public:
  ExprOr(vector<BaseExpr *> vctChild) : BaseExpr(ExprType::EXPR_OR) {
    _vctChild.swap(vctChild);
  }
  ~ExprOr() {
    for (auto ele : _vctChild) {
      delete ele;
    }
  }

  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    bool b = false;
    for (BaseExpr *expr : _vctChild) {
      b = b || expr->CalcBool(vdPara, vdRow);
    }
    return b;
  }

protected:
  vector<BaseExpr *> _vctChild;
};

class ExprIn : public BaseExpr {
public:
  ExprIn(ExprColumn *exprColumn, ExprArray *exprArray)
      : BaseExpr(ExprType::EXPR_IN), _exprColumn(exprColumn),
        _exprArray(exprArray) {}
  ~ExprIn() {
    delete _exprColumn;
    delete _exprArray;
  }

  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _exprColumn->CalcVal(vdPara, vdRow);
    return _exprArray->Exist(pdv);
  }

protected:
  ExprColumn *_exprColumn;
  ExprArray *_exprArray;
};

class ExprLike : public BaseExpr {
public:
  ExprLike(ExprColumn *exprColumn, ExprConst *exprPatten)
      : BaseExpr(ExprType::EXPR_LIKE), _exprColumn(exprColumn),
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

  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprColumn->CalcVal(vdPara, vdRow);

    if (!left->IsStringType()) {
      throw ErrorMsg(DT_UNSUPPORT_OPER,
                     {"LIKE left", StrOfDataType(left->GetDataType())});
    }

    string lStr;
    if (left->GetDataType() == DataType::FIXCHAR)
      lStr = *(DataValueFixChar *)left;
    else
      lStr = *(DataValueVarChar *)left;

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

class ExprNot : public BaseExpr {
public:
  ExprNot(BaseExpr *child) : BaseExpr(ExprType::EXPR_NOT), _child(child) {}
  ~ExprNot() { delete _child; }

  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    return !_child->CalcBool(vdPara, vdRow);
  }

protected:
  BaseExpr *_child;
};

class ExprIsNull : public BaseExpr {
public:
  ExprIsNull(BaseExpr *child)
      : BaseExpr(ExprType::EXPR_ISNULL), _child(child) {}
  ~ExprIsNull() { delete _child; }

  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *pdv = _child->CalcVal(vdPara, vdRow);
    bool b = pdv->IsNull();
    if (!pdv->IsReuse())
      delete pdv;
    return b;
  }

protected:
  BaseExpr *_child;
};

class ExprComp : public BaseExpr {
public:
  ExprComp(CompType type, BaseExpr *l, BaseExpr *r)
      : BaseExpr(ExprType::EXPR_COMP), _compType(type), _exprLeft(l),
        _exprRight(r) {}
  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) override {
    IDataValue *left = _exprLeft->CalcVal(vdPara, vdRow);
    IDataValue *right = _exprRight->CalcVal(vdPara, vdRow);
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
    return false;
  }

protected:
  CompType _compType;
  BaseExpr *_exprLeft;
  BaseExpr *_exprRight;
};

class ExprWhere : public BaseExpr {
public:
  ExprWhere(BaseExpr *child) : BaseExpr(ExprType::EXPR_WHERE), _child(child) {
    assert(_child->GetType() > ExprType::EXPR_SPLIT);
  }
  ~ExprWhere() { delete _child; }
  bool CalcBool(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    if (_child == nullptr)
      return true;

    return _child->CalcBool(vdPara, vdRow);
  }

protected:
  BaseExpr *_child;
  // If the primary index can be used to query, Move the primary index's query
  // condition to here.
  BaseExpr *_priExpr;
  // If
  BaseExpr *_secExpr;
};

class ExprOn : public BaseExpr {
public:
  ExprOn(vector<ExprComp *> vctChild) : BaseExpr(ExprType::EXPR_ON) {
    _vctChild.swap(vctChild);
  }
  ~ExprOn() {
    for (auto c : _vctChild)
      delete c;
  }

protected:
  vector<ExprComp *> _vctChild;
};

class ExprSelect {
public:
protected:
  ExprValueArrayOut *_exprVAout;
  ExprWhere *_where;
};

} // namespace storage
