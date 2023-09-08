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

  // data value type
  EXPR_CONST,
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

  // bool value type
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

  // Statement part
  EXPR_WHERE,
  EXPR_ON,
  EXPR_HAVING,
  EXPR_JOIN,
  EXPR_GROUP_BY,
  EXPR_LIMIT,

  // Input or oupt value and table
  EXPR_COLUMN,
  EXPR_RESULT_COLUMN,
  EXPR_TABLE,
  EXPR_JOIN_TABLE,

  // DDL
  EXPR_CREATE_DATABASE,
  EXPR_DROP_DATABASE,
  EXPR_SHOW_DATABASES,
  EXPR_USE_DATABASE,
  EXPR_CREATE_TABLE,
  EXPR_DROP_TABLE,
  EXPR_SHOW_TABLES,
  EXPR_TRUN_TABLE,
  EXPR_COLUMN_INFO,
  EXPR_CONSTRAINT,
  EXPR_COLUMN_TYPE,

  // Statement
  EXPR_SELECT,
  EXPR_TABLE_SELECT,
  EXPR_INSERT,
  EXPR_UPDATE,
  EXPR_DELETE,

  // Internal function fro SQL. In this version, all function input ExprData*
  // and output ExprData*
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

//
class ExprElem : public BaseExpr {

}
/**
 * @brief Base class for all expression to get, calc data value and return it.
 */
class ExprData : public ExprElem {
public:
  using BaseExpr::BaseExpr;
  /**
   * @brief Get data value from vdLeft or vdRight, then calc them and return the
   * result.
   * @param vdParas The data value array that are inputed from client.
   * @param vdRow The data value array from source table.
   * @return If passed to calc, return the result of data value. If has error,
   * it will return nullptr, the error messagewill be saved in _threadErrorMsg.
   * It will return DataValueNull if the value is null.
   */
  virtual IDataValue *Calc(VectorDataValue &vdParas,
                           VectorDataValue &vdRow) = 0;
};

/**
 * @brief Base class for all aggressive functions, for example count,max,min
 */
class ExprAggr : public ExprElem {
public:
  using BaseExpr::BaseExpr;
  /**
   * @brief Calc the aggressive value
   * @param vdParas The row data value array from source table.
   * @param vdDst the raw data value array to save calculated result
   * @return If return false, it has error in internal,and the error
   * is saved in _threadErrorMsg.
   */
  virtual bool Calc(VectorDataValue &vdParas, VectorDataValue &vdRow,
                    IDataValue &dv) = 0;
};

enum class TriBool : uint8_t { Error = -1, False = 0, True = 1 };
/**
 * @brief Base class for all logic expression, for example and,or
 */
class ExprLogic : public ExprElem {
public:
  using BaseExpr::BaseExpr;
  /**
   * @brief To calc the bool result from child expr and return.
   * @param vdSrc The row data value array from source table.
   * @param vdDst the raw data value array to save calculated result
   * @return TriBool: Error it has error in internal, and the error
   * will be saved in _threadErrorMsg.
   */
  virtual TriBool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) = 0;
};

/**
 * @brief To save array const values, to be used in SQL operator IN.
 */
class ExprArray : public BaseExpr {
public:
  ExprArray() {}
  ~ExprArray() {
    for (IDataValue *pdv : _setVal) {
      delete pdv;
    }
  }

  ExprType GetType() { return ExprType::EXPR_ARRAY; }
  bool Exist(IDataValue *pdv) { return (_setVal.find(pdv) != _setVal.end()); }
  void AddElem(IDataValue *dv) { _setVal.insert(dv); }

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
  ~ExprColumn() { delete _exprData; }
  ExprType GetType() { return ExprType::EXPR_COLUMN; }

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
  MString _name;       // column name
  int _pos;            // The column position in source table columns
  ExprData *_exprData; // The expression to get data value from source
  MString _alias;      // column alias name
};

enum class JoinType {
  JOIN_NULL,
  INNER_JOIN,
  LEFT_JOIN,
  RIGHT_JOIN,
  OUTTER_JOIN
};

class ExprTable : public BaseExpr {
public:
  ExprTable(MString &dbName, MString &name, MString &alias)
      : _dbName(move(dbName)), _tName(move(name)), _tAlias(alias),
        _joinType(JOIN_NULL) {
    if (_tAlias.size() == 0)
      _tAlias = _tName;
  }
  ~ExprTable() {}
  ExprType GetType() { return ExprType::EXPR_TABLE; }

public:
  MString _dbName;
  MString _tName;
  MString _tAlias;
  JoinType _joinType;
};

// Base class for all statement
class ExprStatement : public BaseExpr {
public:
  ExprStatement() {}
  ~ExprStatement() {}
  bool Preprocess() = 0;

public:
  int _paramNum; // The numbe of parameters in this statement
};

} // namespace storage
