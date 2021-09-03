#pragma once
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"

namespace storage {
using namespace std;

class IResultSet {
  /**
   * Move to the first row if possible
   * @return true if moved to the first row; false if failed to move or not able
   * to move to the first row.
   */
  virtual bool First() = 0;
  /**
   * Judge if it has the next
   * @return true if has the next row; false if not have
   */
  virtual bool HasNext() = 0;
  /**
   * Move to the next row if possible
   * @return true if moved to the next row; false if failed to move or not able
   * to move the next row.
   */
  virtual bool Next() = 0;
  /**
   * Move to the last row if possible
   * @return true if moved to the last row; false if failed to move or not able
   * to move the last row.
   */
  virtual bool Last() = 0;
  /**
   * Move to the the row with absolute position if possible
   * @param row the absolute row number, the row number start from 0.
   * @return true if moved to the absolute row; false if failed to move or not
   * able to move the absolute row.
   */
  virtual bool Absolute(int row) = 0;
  /**
   * To judge if the current row is valid
   * @return true the current row is valid and able to read; false the current
   * row number is invalid
   */
  virtual bool IsValidRow() = 0;
  /**
   * Get the rows number if possible
   * @return the total rows number if possible, or -1 if can not get total rows
   * number
   */
  virtual int GetRowCount() = 0;
  /**
   * Get the fields number
   * @return the fields number
   */
  virtual int getFieldsCount() = 0;
  /**
   * Get the filed data type
   * @param fieldIndex the field index, start from 0
   * @return the field data type
   */
  virtual DataType GetFieldDataType(int fieldIndex) = 0;
  /**
   * Get the field name from index
   * @param fieldIndex the field index, start from 0
   * @return the field name
   */
  virtual std::string GetFieldName(int fieldIndex) = 0;
  /**
   * Get field index from name
   * @param fieldName the field name
   * @return the field index
   */
  virtual int GetFieldIndex(string fieldName) = 0;
  /**
   * Get the field long value
   * @param fieldIndex the filed index
   * @return the field long value
   */
  virtual long GetLong(int fieldIndex) = 0;
  /**
   * Get the field int value
   * @param fieldIndex the filed index
   * @return the field int value
   */
  virtual int GetInt(int fieldIndex) = 0;
  /**
   * Get the field boolean value
   * @param fieldIndex the filed index
   * @return the field boolean value
   */
  virtual bool GetBoolean(int fieldIndex) = 0;
  /**
   * Get the field blob value
   * @param fieldIndex the filed index
   * @return the field blob value, the first 4 bytes is the length of blob.The
   * length not include the 4 bytes.
   */
  virtual Byte *GetBlob(int fieldIndex) = 0;
  /**
   * Get the filed string value
   * @param fieldIndex the filed index
   * @return the field string value
   */
  virtual string GetString(int fieldIndex) = 0;
  /**
   * Get the field double value
   * @param fieldIndex the filed index
   * @return the field double value
   */
  virtual double GetDouble(int fieldIndex) = 0;
  /**
   * Get the field float value
   * @param fieldIndex the filed index
   * @return the field float value
   */
  virtual float GetFloat(int fieldIndex) = 0;
  /**
   * Get the field date value
   * @param fieldIndex the filed index
   * @return the field double value
   */
  virtual time_t GetDate(int fieldIndex) = 0;
  /**
   * Get the field short value
   * @param fieldIndex the filed index
   * @return the field float value
   */
  virtual short GetShort(int fieldIndex) = 0;
  /**
   * Get IDataValue
   * @param fieldIndex the filed index
   * @return the field value
   */
  virtual IDataValue *GetDataValue(int fieldIndex) = 0;
  /**
   * Get the field long value from field name
   * @param fieldName the field name
   * @return the field long value
   */
  virtual long GetLong(string fieldName) = 0;
  /**
   * Get the field int value from field name
   * @param fieldName the field name
   * @return the field int value
   */
  virtual int GetInt(string fieldName) = 0;
  /**
   * Get the field boolean value from field name
   * @param fieldName the field name
   * @return the field boolean value
   */
  virtual bool GetBoolean(string fieldName) = 0;
  /**
   * Get the field blob value from field name
   * @param fieldName the field name
   * @return the field blob value, the first 4 bytes is the length of blob, not
   * include the 4 bytes.
   */
  virtual Byte *GetBlob(string fieldName) = 0;
  /**
   * Get the field string value from field name
   * @param fieldName the field name
   * @return the field string value
   */
  virtual string GetString(string fieldName) = 0;
  /**
   * Get the field double value from field name
   * @param fieldName the field name
   * @return the field double value
   */
  virtual double GetDouble(string fieldName) = 0;
  /**
   * Get the field float value from field name
   * @param fieldName the field name
   * @return the field float value
   */
  virtual float GetFloat(string fieldName) = 0;
  /**
   * Get the field date value from field name
   * @param fieldName the field name
   * @return the field double value
   */
  virtual time_t getDate(string fieldName) = 0;
  /**
   * Get the field short value from field name
   * @param fieldName the field name
   * @return the field float value
   * @throws StorageInvalidDataTypeException
   * @throws StorageInvalidFiledNameException
   */
  virtual short GetShort(string fieldName) = 0;
  /**
   * Get a field DataValue
   * @param fieldName the field name
   * @return the field value
   * @throws StorageInvalidFiledNameException
   */
  virtual IDataValue *GetDataValue(string fieldName) = 0;

  /**
   * Get the current row with DataValue type
   * @return
   */
  virtual void GetCurrDataValueRow(VectorDataValue &vct) = 0;
  virtual void close() {}
};

} // namespace storage
