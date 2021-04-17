#pragma once
#include "../core/ActionType.h"
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include "IResultSet.h"
#include <future>
#include <string>

namespace storage {
class Statement {
public:
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
  virtual DataType GetDataType(int paraIndex) = 0;

  /**
   * Get the max length for a field
   *
   * @param paraIndex the field index, start from 0;
   * @return the max length
   */
  virtual int GetMaxLength(int paraIndex) = 0;

  /**
   * Get the filed if can input null value
   *
   * @param paraIndex the field index, start from 0;
   * @return
   */
  virtual bool IsNullable(int paraIndex) = 0;

  /**
   * set a long value
   *
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetLong(int paraIndex, long val) = 0;

  /**
   * Set a int value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetInt(int paraIndex, int val) = 0;

  /**
   * Set a boolean value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   * @throws StorageInvalidDataTypeException
   */
  virtual void SetBoolean(int paraIndex, bool val) = 0;

  /**
   * Set a string value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   * @throws StorageInvalidDataTypeException
   * @throws StorageOverLengthException
   */
  virtual void SetString(int paraIndex, const std::string &val) = 0;

  /**
   * Set a byte array value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetBlob(int paraIndex, char *val) = 0;

  /**
   * Set a double value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetDouble(int paraIndex, double val) = 0;

  /**
   * Set a float value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetFloat(int paraIndex, float val) = 0;

  /**
   * Set a Date value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetDate(int paraIndex, time_t val) = 0;

  /**
   * Set a float value
   *
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  virtual void SetShort(int paraIndex, short val) = 0;

  /**
   * Set a value by index
   *
   * @param paraIndex fieldName the field name
   * @param val       the value to set
   */
  virtual void SetDataValue(int paraIndex, IDataValue *val) = 0;

  /**
   * One time set all fields values, order by fields order
   *
   * @param vals
   */
  virtual void SetValues(VectorDataValue &vals) = 0;

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
}
} // namespace storage
