#pragma once
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../core/ActionType.h"
#include "../dataType/DataType.h"
#include "../dataType/DataValueBlob.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include "../resultset/IResultSet.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include "Connection.h"

#include <atomic>
#include <chrono>
#include <future>

namespace storage {
class Transaction;
class StatementClient {
public:
  /**
   * Constuctor for statement
   * @param tran The transaction own this statement, maybe null and create it
   * automately
   * @param vct Used to save the parameters types, maxlength etc.
   * @param statTime If save create, execute and stop time for statistics
   */
  StatementClient(Connection *conn) : _conn(conn) { assert(conn != nullptr); }

  ~StatementClient() {}

  /**
   * set a long value
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetLong(int paraIndex, int64_t val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueLong(val, false)});
    assert(pair.second);
  }

  /**
   * Set a int value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetInt(int paraIndex, int32_t val) {
    auto pair = _mapDataValue.insert({paraIndex, new DataValueInt(val, false)});
    assert(pair.second);
  }

  /**
   * Set a short value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetShort(int paraIndex, int16_t val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueShort(val, false)});
    assert(pair.second);
  }

  /**
   * Set a char value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetChar(int paraIndex, char val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueChar(val, false)});
    assert(pair.second);
  }

  /**
   * set a ulong value
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetULong(int paraIndex, uint64_t val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueULong(val, false)});
    assert(pair.second);
  }

  /**
   * Set a uint value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetUInt(int paraIndex, uint32_t val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueUInt(val, false)});
    assert(pair.second);
  }

  /**
   * Set a ushort value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetUShort(int paraIndex, uint16_t val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueUShort(val, false)});
    assert(pair.second);
  }

  /**
   * Set a Byte value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetByte(int paraIndex, Byte val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueByte(val, false)});
    assert(pair.second);
  }

  /**
   * Set a boolean value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetBool(int paraIndex, bool val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueBool(val, false)});
    assert(pair.second);
  }

  /**
   * Set a string value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetString(int paraIndex, const MString &val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueVarChar(val, false)});
    assert(pair.second);
  }

  /**
   * Set a byte array value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param len the length for blob
   * @param val       the value to set
   */
  void SetBlob(int paraIndex, uint32_t len, char *val) {
    auto pair = _mapDataValue.insert({paraIndex, new DataValueBlob(val, len)});
    assert(pair.second);
  }

  /**
   * Set a double value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetDouble(int paraIndex, double val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueDouble(val, false)});
    assert(pair.second);
  }

  /**
   * Set a float value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetFloat(int paraIndex, float val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueFloat(val, false)});
    assert(pair.second);
  }

  /**
   * Set a Date value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the milliseconds from the start of the Clock's epoch.
   */
  void SetDate(int paraIndex, uint64_t val) {
    auto pair =
        _mapDataValue.insert({paraIndex, new DataValueDate(val, false)});
    assert(pair.second);
  }

  /**
   * load parameters from byte array, maybe 1~N rows.
   * param bys: The byte array, the struct is below:
   * 4 bytes how many rows in bytes array, 2 bytes how many fields in one
   row.
   * following are the values for all parameters.
   * param len: bys length, used to verify byte array.
   */
  void LoadParams(Byte *bys, int len) {
    int rows = *(int *)bys;
    bys += sizeof(int);
    len -= sizeof(int);
    int fields = *(short *)bys;
    bys += sizeof(short);
    len -= sizeof(short);

    if (fields != _vctParaTmpl->size()) {
      throw ErrorMsg(TRAN_ADD_TASK_FAILED, {});
    }

    for (int i = 0; i < rows; i++) {
      for (IDataValue *dv : _vctPara) {
        int rl = dv->ReadData(bys, 0);
        bys += rl;
        len -= rl;
      }
    }

    if (len != 0) {
      throw ErrorMsg(TRAN_ADD_TASK_FAILED, {});
    }
  }
  /**
   * Add current row to a batch array, all rows in a batch will submit one
   time
   */
  void AddBatch() {
    VectorDataValue *vct = new VectorDataValue();
    vct->swap(_vctPara);
    _vctRow.push_back(vct);

    _vctPara.reserve(_vctParaTmpl->size());
    for (auto &dv : *_vctParaTmpl) {
      _vctPara.push_back(dv->Clone());
    }
  }

  /**
   * Submit one row or a batch of rows to insert, update or delete
   * @return How much records have been affected.
   */
  virtual int ExecuteUpdate() { abort(); }

  /**
   * Execute query statement and return the result
   * @return The query result
   */
  virtual IResultSet *ExecuteQuery() { abort(); }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  // To save one row's data value with type map<index, dv>
  map<int, IDataValue *> _mapDataValue;
  // To save rows of data values when batch type
  VectorRow _vctRow;
  // The cost microseconds to execute this statement.
  uint32_t _costTime = 0;
  // The connection that own this statement
  Connection *_conn;
  // a new transaction.
  bool _bAutoTran = false;
};
} // namespace storage
