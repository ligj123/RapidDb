#pragma once
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../expr/BaseExpr.h"

#include <utility>

namespace storage {
using namespace std;

class IResultSet {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  IResultSet(MVectorPtr<ExprColumn *> *vctCol) : _vctCol(vctCol) {
    for (ExprColumn *ecol : *_vctCol) {
      _mapColPos.emplace(*ecol->_name, ecol->_pos);
    }
  }

  virtual void AddRow(VectorDataValue &&vctDv) { abort(); }
  virtual void AddRow(VectorDataValue &&vctDv, LeafRecord *lr) { abort(); }
  /**
   * Move to the first row if possible
   * @return true if moved to the first row; false if failed to move or not
   * able to move to the first row.
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
  virtual int64_t GetRowCount() = 0;
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
  virtual MString &GetFieldName(int fieldIndex) = 0;
  /**
   * Get field index from name
   * @param fieldName the field name
   * @return the field index
   */
  virtual int GetFieldIndex(MString &fieldName) = 0;

  /**
   * Get IDataValue
   * @param fieldIndex the filed index
   * @return the field value
   */
  virtual IDataValue *GetDataValue(int fieldIndex) = 0;

  /**
   * Get a field DataValue
   * @param fieldName the field name
   * @return the field value
   * @throws StorageInvalidFiledNameException
   */
  virtual IDataValue *GetDataValue(MString &fieldName) = 0;

  /**
   * Get the current row with DataValue type
   * @return
   */
  virtual bool GetCurrDataValueRow(VectorDataValue &vct) = 0;
  virtual void close() {}

protected:
  MVectorPtr<ExprColumn *> *_vctCol;
  MHashMap<MString, int> _mapColPos;
};

} // namespace storage
