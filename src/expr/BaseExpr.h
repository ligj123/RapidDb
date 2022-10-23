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
  EXPR_COMP,
  EXPR_IN OR_NOT,
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

  // Input or oupt value and table
  EXPR_COLUMN,
  EXPR_TABLE,

  // Select
  EXPR_TABLE_SELECT,
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

//
class ExprLogic : public BaseExpr {
public:
  using BaseExpr::BaseExpr;
  // The expression below EXPR_SPLIT to call this functionto calc and return
  // bool value.
  virtual bool Calc(VectorDataValue &vdPara, VectorDataValue &vdRow) = 0;
};

// The base class for Columns of input or output
class ExprColumn : public BaseExpr {
public:
  ExprColumn(int index, string &&colName, DataType dt, ExprData *data,
             string &&colAlias)
      : _index(index), _colName(move(colName)), _colAlias(move(colAlias)),
        _dataType(dt), _data(data) {
    if (_colAlias.size() == 0)
      _colAlias = _colName;
  }
  ~ExprColumn() { delete _data; }

  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) {
    IDataValue *pDv = _data->Calc(vdSrc, vdDst);
    vdDst[_index]->Copy(*pDv, !pDv->IsReuse());
    if (!pDv->IsReuse())
      delete pDv;
  }
  ExprType GetDataType() { return ExprType::EXPR_COLUMN; }
  int GetIndex() { return _index; }
  string &GetColumnName() { return _colName; }
  string GetColumnAlias() { return _colAlias; }

protected:
  // The index in table's columns array
  int _index;
  // Column name
  string _colName;
  // Column alias
  string _colAlias;
  // The related data type for this column
  DataType _dataType;
  // For no const type, to tell how to get parameter from array.
  ExprData *_data;
};

class ExprTable : public BaseExpr {
public:
  ExprTable(string &name, string &alias, vector<ExprColumn> vctCol)
      : _tableName(name), _tableAlias(alias) {
    _vctCol.swap(vctCol);
    if (_tableAlias.size() == 0)
      _tableAlias = _tableName;
  }
  ~ExprTable() {}

  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) {
    for (auto &col : _vctCol) {
      col.Calc(vdSrc, vdDst);
    }
  }

  const string &GetTableName() { return _tableName; }
  const string &GetTableAlias() { return _tableAlias; }

protected:
  string _tableName;
  string _tableAlias;
  vector<ExprColumn> _vctCol;
};
} // namespace storage
