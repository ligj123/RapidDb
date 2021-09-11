#pragma once
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
#include <chrono>
#include <future>
#include <string>

namespace storage {
class Transaction;
class Statement {
public:
  Statement() {
    if (_bStatTime)
      _createTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  }
  ~Statement() {}
  /**
   * Get which action will to do
   *
   * @return action type
   */
  virtual ActionType GetActionType() { return ActionType::UNKNOWN; }

  /**
   * Get the field's data type
   *
   * @param paraIndex the field index, start from 0;
   * @return data type
   */
  virtual DataType GetDataType(int paraIndex) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    return _vctPara[paraIndex]->GetDataType();
  };

  /**
   * Get the max length for a field
   *
   * @param paraIndex the field index, start from 0;
   * @return the max length
   */
  virtual int GetMaxLength(int paraIndex) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    return _vctPara[paraIndex]->GetMaxLength();
  }

  /**
   * set a long value
   *
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetLong(int paraIndex, int64_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::LONG)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"LONG", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueLong *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a int value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetInt(int paraIndex, int32_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::INT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"INT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueInt *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a short value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetShort(int paraIndex, int16_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::SHORT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"SHORT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueShort *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a char value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetChar(int paraIndex, char val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::SHORT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"SHORT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueChar *)_vctPara[paraIndex] = val;
  }

  /**
   * set a ulong value
   *
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetULong(int paraIndex, uint64_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::ULONG)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"ULONG", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueULong *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a uint value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetUInt(int paraIndex, uint32_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::UINT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"UINT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueUInt *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a ushort value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetUShort(int paraIndex, uint16_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::USHORT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"USHORT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueUShort *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a Byte value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetByte(int paraIndex, Byte val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::BYTE)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"BYTE", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueByte *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a boolean value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   * @throws StorageInvalidDataTypeException
   */
  virtual void SetBool(int paraIndex, bool val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::BOOL)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"BOOL", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueBool *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a string value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   * @throws StorageInvalidDataTypeException
   * @throws StorageOverLengthException
   */
  virtual void SetString(int paraIndex, const std::string &val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (!_vctPara[paraIndex]->IsStringType())
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"STRING", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    if (val.size() > _vctPara[paraIndex]->GetMaxLength())
      throw ErrorMsg(ErrorID::EXPR_EXCEED_MAX_LENGTH,
                     {to_string(val.size()),
                      to_string(_vctPara[paraIndex]->GetMaxLength())});
    if (_vctPara[paraIndex]->GetDataType() == DataType::FIXCHAR)
      *(DataValueFixChar *)_vctPara[paraIndex] = val;
    else
      *(DataValueVarChar *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a byte array value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param len the length for blob
   * @param val       the value to set
   */
  virtual void SetBlob(int paraIndex, uint32_t len, char *val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::BLOB)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"BLOB", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    if (len > _vctPara[paraIndex]->GetMaxLength())
      throw ErrorMsg(
          ErrorID::EXPR_EXCEED_MAX_LENGTH,
          {to_string(len), to_string(_vctPara[paraIndex]->GetMaxLength())});
    ((DataValueBlob *)_vctPara[paraIndex])->Put(len, val);
  }

  /**
   * Set a double value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetDouble(int paraIndex, double val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::DOUBLE)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"DOUBLE", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueDouble *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a float value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetFloat(int paraIndex, float val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::FLOAT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"FLOAT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueFloat *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a Date value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the milliseconds from the start of the Clock's epoch.
   */
  virtual void SetDate(int paraIndex, uint64_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::DATETIME)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"DATETIME", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueDate *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a value by index
   *
   * @param paraIndex fieldName the field name
   * @param val       the value to set
   */
  virtual void SetDataValue(int paraIndex, const IDataValue &val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});

    _vctPara[paraIndex]->Copy(val);
  }

  /**
   * One time set all fields values, order by fields order
   *
   * @param vals, the data values will be copied, not moved
   */
  virtual void SetValues(const VectorDataValue &vals) {
    if (vals.size() != _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(vals.size()), to_string(_vctPara.size())});
    for (int i = 0; i < vals.size(); i++)
      _vctPara[i]->Copy(*vals[i]);
  }

  /**
   * Add current row to a batch array, all rows in a batch will submit one time
   */
  virtual void AddBatch() = 0;

  /**
   * Submit one row or a batch of rows to insert, update or delete
   *
   * @return How much records have been affected.
   */
  virtual int ExecuteUpdate() { return 0; }

  virtual future<int> ExecuteUpdateAsyn() {
    return std::promise<int>().get_future();
  }

  /**
   * Submit one row or a batch of rows to insert, update or delete
   *
   * @param vals
   * @return How much records have been affected.
   */
  virtual int ExecuteUpdate(VectorDataValue &vals) { return 0; }

  virtual future<int> ExecuteUpdateAsyn(VectorDataValue &vals) {
    return std::promise<int>().get_future();
  }
  /**
   * Execute query statement and return the result
   *
   * @return The query result
   */
  virtual IResultSet *ExecuteQuery() { return nullptr; }

  /**
   * Execute query statement and return the result
   *
   * @return The query result
   */
  virtual IResultSet *ExecuteQuery(VectorDataValue &vals) { return nullptr; }

  /**
   * Close this instance
   */
  virtual void Close() {}

  std::chrono::nanoseconds GetCreateTime() { return _createTime; }
  std::chrono::nanoseconds GetStartTime() { return _startTime; }
  std::chrono::nanoseconds GetStopTime() { return _stopTime; }
  bool IsFinished() { return _tinyTasks == _finishedTask; }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  VectorDataValue _vctPara;
  // The create time for this statement
  // If multi statement to update one record, they will run one by one according
  // create time
  std::chrono::nanoseconds _createTime;
  // The start time to execute for this statement
  std::chrono::nanoseconds _startTime;
  // The finished or abort time to execute for this statement
  std::chrono::nanoseconds _stopTime;
  // The number of tiny taks. One statement maybe splite serveral tiny tasks to
  // run. Here used to save how much tiny tasks in total.
  int _tinyTasks = 0;
  // The number of finished tiny tasks.
  int _finishedTask = 0;
  // The transaction to run this task, not nullable.
  Transaction *_transaction;
  // If get the time for statistics.
  bool _bStatTime = false;
};
} // namespace storage
