#pragma once
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../table/Table.h"
#include "../utils/ErrorMsg.h"
#include <unordered_set>
#include <vector>

using namespace std;
namespace storage {
enum class ExprType {
  EXPR_BASE,
  // const type
  EXPR_ARRAY,
  EXPR_CONST,

  // return data value
  EXPR_PARAMETER,
  EXPR_FIELD,
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
  EXPR_COMP,
  EXPR_IN_OR_NOT,
  EXPR_IS_NULL_NOT,
  EXPR_BETWEEN,
  EXPR_LIKE,
  EXPR_EXIST,
  EXPR_NOT_EXIST,
  EXPR_AND,
  EXPR_OR,
  EXPR_NOT,

  EXPR_WHERE,
  EXPR_ON,
  EXPR_HAVING,
  EXPR_JOIN,
  EXPR_GROUP_BY,

  // DDL
  EXPR_CREATE_DATABASE,
  EXPR_DROP_DATABASE,
  EXPR_SHOW_DATABASES,
  EXPR_USE_DATABASE,
  EXPR_CREATE_TABLE,
  EXPR_DROP_TABLE,

  // Input or oupt value and table
  EXPR_COLUMN,
  EXPR_RESULT_COLUMN,
  EXPR_TABLE,
  EXPR_JOIN_TABLE,

  // Select
  EXPR_SELECT,
  EXPR_TABLE_SELECT,

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
  virtual ExprType GetType() = 0;

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
};

/**
 * @brief Base class for all expression to get, calc data value and return it.
 */
class ExprData : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  /**
   * @brief Get data value from vdLeft or vdRight, then calc them and return the
   * result.
   * @param vdLeft The data value array that are inputed from client or left
   * table (join table).
   * @param vdRight The data value array from table or right table (join table).
   * @return If the result is directly from vdLeft or vdRight, it will be marked
   * as resued and does not need to free. If it is calculated result, it will
   * need to free it. If return nullptr, it has error in internal and the error
   * is saved in _threadErrorMsg.
   */
  virtual IDataValue *Calc(VectorDataValue &vdLeft,
                           VectorDataValue &vdRight) = 0;
};

/**
 * @brief Base class for all aggressive expression, for example count,max,min
 */
class ExprAggr : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  /**
   * @brief Load data value from vdSrc and calc the result, then save it into
   * vdDst
   * @param vdSrc The row data value array from source table.
   * @param vdDst the raw data value array to save calculated result
   * @return If return false, it has error in internal,and the error
   * is saved in _threadErrorMsg.
   */
  virtual bool Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) = 0;
};

enum class TriBool { Error = -1, False = 0, True };
/**
 * @brief Base class for all logic expression, for example and,or
 */
class ExprLogic : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  /**
   * @brief To calc the bool result from child expr and return.
   * @param vdSrc The row data value array from source table.
   * @param vdDst the raw data value array to save calculated result
   * @return TriBool:Error it has error in internal,and the error
   * is saved in _threadErrorMsg.
   */
  virtual TriBool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) = 0;
};

/**
 * @brief To save array const values, to be used in SQL operator IN.
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

public:
  unordered_set<IDataValue *, DataValueHash, DataValueEqual,
                Mallocator<IDataValue *>>
      _setVal;
};

/**
 * @brief The column information for insert or update. Here will use this to
 * get the data value from exist row data or parameters, then save it into row
 * data value array
 */
class ExprColumn : public BaseExpr {
public:
  ExprColumn(string &name) : _name(move(name)), _pos(-1), _exprData(nullptr) {}
  ~ExprColumn() { delete _exprData; }
  ExprType GetType() { return ExprType::EXPR_COLUMN; }
  void Preprocess(int pos, ExprData *exprData) {
    _pos = pos;
    _exprData = exprData;
  }

  bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) {
    IDataValue *dv = _exprData->Calc(vdPara, vdRow);
    if (dv == nullptr)
      return false;

    vdRow[_pos]->Copy(*dv, !dv->IsReuse());
    if (!dv->IsReuse()) {
      delete dv;
    }

    return true;
  }

public:
  string _name;        // column name
  int _pos;            // The position in source table columns
  ExprData *_exprData; // The expression to get data value from source
};

// The column in temp table for select result.
class ExprResColumn : public ExprColumn {
public:
  ExprResColumn(string &tname, string &name, string &alias)
      : ExprColumn(name), _tName(move(tname)), _alias(move(alias)) {}
  ~ExprResColumn() {}
  ExprType GetDataType() const { return ExprType::EXPR_RESULT_COLUMN; }
  /**
   * @brief Load data value from left or right source table, and calculate and
   * get the last result, then save it into dest table
   * @param vdLeft The vector of data values from left source table
   * @param vdRight The vector of data values from right source table
   * @param vdDest The vector of data value for destion table
   */
  bool Calc(VectorDataValue &vdLeft, VectorDataValue &vdRight,
            VectorDataValue &vdDest) {
    IDataValue *dv = _exprData->Calc(vdLeft, vdRight);
    if (dv == nullptr)
      return false;

    vdDest[_pos]->Copy(*dv, !dv->IsReuse());
    if (!dv->IsReuse()) {
      delete dv;
    }
    return false;
  }

public:
  string _tName; // table name
  string _alias; // alias name
};

} // namespace storage
