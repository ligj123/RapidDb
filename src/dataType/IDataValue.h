#pragma once
#include "../cache/CachePool.h"
#include "../cache/StrBuff.h"
#include "DataType.h"
#include <any>
#include <cassert>
#include <cstring>
#include <fstream>

namespace storage {
class IDataValue {
public:
  static bool IsIndexType(const DataType dt) {
    return ((int)dt & (int)DataType::INDEX_TYPE) == (int)DataType::INDEX_TYPE;
  }

  static bool IsFixLength(const DataType dt) {
    return ((int)dt & (int)DataType::FIX_LEN) == (int)DataType::FIX_LEN;
  }

  static bool IsAutoPrimaryKey(const DataType dt) {
    return ((int)dt & (int)DataType::AUTO_INC_TYPE) ==
           (int)DataType::AUTO_INC_TYPE;
  }

  static bool IsDigital(const DataType dt) {
    return ((int)dt & (int)DataType::DIGITAL_TYPE) ==
           (int)DataType::DIGITAL_TYPE;
  }

  static bool IsArrayType(const DataType dt) {
    return ((int)dt & (int)DataType::ARRAY_TYPE) == (int)DataType::ARRAY_TYPE;
  }

public:
  bool IsIndexType() {
    return ((int)dataType_ & (int)DataType::INDEX_TYPE) ==
           (int)DataType::INDEX_TYPE;
  }

  bool IsFixLength() {
    return ((int)dataType_ & (int)DataType::FIX_LEN) == (int)DataType::FIX_LEN;
  }

  bool IsAutoPrimaryKey() {
    return ((int)dataType_ & (int)DataType::AUTO_INC_TYPE) ==
           (int)DataType::AUTO_INC_TYPE;
  }

  bool IsDigital() {
    return ((int)dataType_ & (int)DataType::DIGITAL_TYPE) ==
           (int)DataType::DIGITAL_TYPE;
  }

  bool IsArrayType() {
    return ((int)dataType_ & (int)DataType::ARRAY_TYPE) ==
           (int)DataType::ARRAY_TYPE;
  }

  bool IsStringType() {
    return dataType_ == DataType::FIXCHAR || dataType_ == DataType::VARCHAR;
  }

  IDataValue(const IDataValue &dv)
      : dataType_(dv.dataType_), valType_(dv.valType_), bKey_(dv.bKey_) {}
  IDataValue(DataType dataType, ValueType valType, const bool bKey)
      : dataType_(dataType), valType_(valType), bKey_(bKey) {}
  virtual ~IDataValue() {}
  /**
   * return the data type for this data value
   */
  DataType GetDataType() const { return dataType_; }
  ValueType GetValueType() const { return valType_; }
  bool IsNull() { return valType_ == ValueType::NULL_VALUE; }
  bool isKey() { return bKey_; }

  virtual IDataValue *CloneDataValue(bool incVal = false) = 0;
  virtual std::any GetValue() const = 0;
  virtual uint32_t WriteData(Byte *buf) = 0;
  virtual uint32_t ReadData(Byte *buf, uint32_t len, bool bSole = true) = 0;
  virtual uint32_t WriteData(Byte *buf, bool key) = 0;
  virtual uint32_t WriteData(fstream &fs) {
    assert(false);
    return 0;
  }
  virtual uint32_t ReadData(fstream &fs) {
    assert(false);
    return 0;
  }
  /**The memory size to save data*/
  virtual uint32_t GetDataLength() const = 0;
  /**The max memory size that can bu used to save this data*/
  virtual uint32_t GetMaxLength() const = 0;
  /**How much bytes to save this data to disk*/
  virtual uint32_t GetPersistenceLength() const = 0;
  virtual uint32_t GetPersistenceLength(bool key) const = 0;
  virtual void SetMinValue() = 0;
  virtual void SetMaxValue() = 0;
  virtual void SetDefaultValue() = 0;
  virtual void ToString(StrBuff &sb) = 0;

  friend std::ostream &operator<<(std::ostream &os, const IDataValue &dv);
  friend bool operator==(const IDataValue &dv1, const IDataValue &dv2);

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  DataType dataType_;
  ValueType valType_;
  bool bKey_;
};

class VectorDataValue : public vector<IDataValue *> {
public:
  using vector::vector;
  ~VectorDataValue() {
    for (auto iter = begin(); iter != end(); iter++) {
      delete *iter;
    }
  }

  void RemoveAll() {
    for (auto iter = begin(); iter != end(); iter++) {
      delete *iter;
    }

    clear();
  }
};

class VectorRow : public vector<VectorDataValue *> {
public:
  using vector::vector;
  ~VectorRow() {
    for (auto iter = begin(); iter != end(); iter++) {
      delete *iter;
    }
  }

  void RemoveAll() {
    for (auto iter = begin(); iter != end(); iter++) {
      delete *iter;
    }

    clear();
  }
};
} // namespace storage
