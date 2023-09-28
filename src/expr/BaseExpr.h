#pragma once
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "ExprType.h"

#include <unordered_set>
#include <vector>

using namespace std;
namespace storage {
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

// Base class for ExprData, ExprLogic, ExprAggr
class ExprElem : public BaseExpr {};

/**
 * @brief Base class for all expression to get, calc data value and return it.
 */
class ExprData : public ExprElem {
public:
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

enum class TriBool : int8_t { Error = -1, False = 0, True = 1 };
/**
 * @brief Base class for all logic expression, for example and,or
 */
class ExprLogic : public ExprElem {
public:
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
      pdv->DecRef();
    }
  }

  ExprType GetType() override { return ExprType::EXPR_ARRAY; }
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
  ExprColumn(MString *name, ExprElem *exprElem, MString *alias)
      : _name(name), _exprElem(exprElem), _alias(alias) {}
  ~ExprColumn() {
    delete _exprElem;
    delete _name;
    delete _alias;
  }

  ExprType GetType() override { return ExprType::EXPR_COLUMN; }

public:
  MString *_name;      // column name
  int _pos;            // The column position in source table columns
  ExprElem *_exprElem; // The expression to get data value from source
  MString *_alias;     // column alias name
};

class ExprTable : public BaseExpr {
public:
  ExprTable(MString *dbName, MString *tName, MString *_tAlias = nullptr)
      : _dbName(dbName), _tName(tName), _tAlias(_tAlias) {}
  ~ExprTable() {
    delete _dbName;
    delete _tName;
    delete _tAlias;
  }

  ExprType GetType() override { return ExprType::EXPR_TABLE; }

public:
  MString *_dbName;
  MString *_tName;
  MString *_tAlias;
  JoinType _joinType{JoinType::JOIN_NULL};
};

// Base class for all statement
class ExprStatement : public BaseExpr {
public:
  virtual bool Preprocess() = 0;

public:
  // int _paramNum{0}; // The numbe of parameters in this statement
};

} // namespace storage
