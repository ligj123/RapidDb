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
             int dataBasicStart, int prevVarCols, int colNullPlace)
      : _index(index), _colName(move(colName)), _dataType(dt), _data(data),
        _dataBasicStart(dataBasicStart), _prevVarCols(prevVarCols),
        _colNullPlace(colNullPlace) {}
  ~ExprColumn() { delete _data; }

  void Calc(VectorDataValue &vdSrc, VectorDataValue &vdDst) {
    IDataValue *pDv = _data->Calc(vdSrc, vdDst);
    vdDst[_index]->Copy(*pDv, !pDv->IsReuse());
    if (!pDv->IsReuse())
      delete pDv;
  }

  ExprType GetDataType() { return ExprType::EXPR_COLUMN; }
  int GetIndex() { return _index; }
  const string &GetColumnName() { return _colName; }
  const int GetDataBasicStart() const { return _dataBasicStart; }
  const int GetPrevVarCols() const { return _prevVarCols; }
  const int GetColNullPlace() const { return _colNullPlace; }

  /** Return the column's data start position, if is null, return -1
   * @param bys the begin address for this record.
   * @return -1: if value is null; >= 0: the actual start position of column
   * data
   */
  int CalcPosition(Byte *bys) {
    if ((bys[sizeof(int32_t) + _index / BYTE_SIZE] &
         (Byte)(1 << (_index % BYTE_SIZE))) == 0) {
      return -1;
    } else {
      return _dataBasicStart +
             (_prevVarCols <= 0
                  ? 0
                  : UInt32FromBytes(bys + _colNullPlace +
                                    _prevVarCols * sizeof(int32_t)));
    }
  }
  /**To judge if a column'svalue in a row is null*/
  bool IsNull(Byte *bys) {
    return bys[_index / BYTE_SIZE] & (1 << (_index % BYTE_SIZE));
  }

  /**
   * Return the data length for this column
   * @param bys the row data begin bytes.
   * @return the actual data length, if null, return 0;
   */
  int GetLength(Byte *bys) {
    if ((bys[UI32_LEN + _index / BYTE_SIZE] &
         (Byte)(1 << (_index % BYTE_SIZE))) == 0) {
      return 0;
    }

    switch (_dataType) {
    case DataType::LONG:
    case DataType::ULONG:
    case DataType::DOUBLE:
    case DataType::DATETIME:
      return 8;
    case DataType::FLOAT:
    case DataType::INT:
    case DataType::UINT:
      return 4;
    case DataType::SHORT:
    case DataType::USHORT:
      return 2;
    case DataType::BOOL:
    case DataType::CHAR:
    case DataType::BYTE:
      return 1;
    case DataType::FIXCHAR:
    case DataType::VARCHAR:
    case DataType::BLOB: {
      int pos = sizeof(int32_t) + _colNullPlace;
      return (_prevVarCols == 0
                  ? UInt32FromBytes(bys + pos)
                  : UInt32FromBytes(bys + pos + _prevVarCols * UI32_LEN) -
                        UInt32FromBytes(bys + pos + _prevVarCols * UI32_LEN -
                                        UI32_LEN));
    }
    default:
      return -1;
    }
  }
  /**
   * compare the same column for two records
   * @param bys1
   * @param bys2
   * @return
   */
  int CompareTo(Byte *bys1, Byte *bys2) {
    int pos1 = CalcPosition(bys1);
    int pos2 = CalcPosition(bys2);
    if (pos1 < 0 && pos2 < 0) {
      return 0;
    } else if (pos1 < 0) {
      return -1;
    } else if (pos2 < 0) {
      return 1;
    }

    Byte *b1 = bys1 + pos1;
    Byte *b2 = bys2 + pos2;
    switch (_dataType) {
    case DataType::LONG:
      return (int)(*((int64_t *)b1) - *((int64_t *)b2));
    case DataType::ULONG:
      return (int)(*((uint64_t *)b1) - *((uint64_t *)b2));
    case DataType::DOUBLE:
      return (int)(*((double *)b1) - *((double *)b2));
    case DataType::DATETIME:
      return (int)(*((uint64_t *)b1) - *((uint64_t *)b2));
    case DataType::FLOAT:
      return (int)(*((float *)b1) - *((float *)b2));
    case DataType::INT:
      return *((int32_t *)b1) - *((int32_t *)b2);
    case DataType::UINT:
      return *((uint32_t *)b1) - *((uint32_t *)b2);
    case DataType::SHORT:
      return *((int16_t *)b1) - *((int16_t *)b2);
    case DataType::USHORT:
      return *((uint16_t *)b1) - *((uint16_t *)b2);
    case DataType::BOOL:
      return (*b1 ? 1 : 0) - (*b2 ? 1 : 0);
    case DataType::CHAR:
      return *((char *)b1) - *((char *)b2);
    case DataType::BYTE:
      return *b1 - *b2;
    case DataType::FIXCHAR:
    case DataType::VARCHAR:
    case DataType::BLOB:
      return BytesCompare(b1, GetLength(bys1), b2, GetLength(bys2));
    default:
      throw new ErrorMsg(DT_UNKNOWN_TYPE, {to_string((uint32_t)_dataType)});
    }
  }

protected:
  // The index in table's columns array
  int _index;
  // Column name
  string _colName;
  // The related data type for this column
  DataType _dataType;
  // For no const type, to tell how to get parameter from array.
  ExprData *_data;
  // Below 3 variable used to fast pick up a field in records.
  // The structure for a record data:
  // 1. n bytes to save if the columns' values are null, 8 columns per byte
  // 2. 2 bytes * n variable columns, to save total length of variable columns
  // before and include this column
  // 3. the actual value for every columns, if value is null, not occupy bytes.
  /** The data start position in a record value byte array.
   * This value does not include the variable columns' length.
   */
  int _dataBasicStart;
  /**To save how many variable columns(String or blob) before this column*/
  int _prevVarCols;
  /**How many bytes to save if the columns are null, 8 columns per byte*/
  int _colNullPlace;
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
