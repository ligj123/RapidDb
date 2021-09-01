#pragma once
#include "../cache/CachePool.h"
#include "../cache/Mallocator.h"
#include "../cache/StrBuff.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
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
  bool IsIndexType() const {
    return ((int)dataType_ & (int)DataType::INDEX_TYPE) ==
           (int)DataType::INDEX_TYPE;
  }

  bool IsFixLength() const {
    return ((int)dataType_ & (int)DataType::FIX_LEN) == (int)DataType::FIX_LEN;
  }

  bool IsAutoPrimaryKey() const {
    return ((int)dataType_ & (int)DataType::AUTO_INC_TYPE) ==
           (int)DataType::AUTO_INC_TYPE;
  }

  bool IsDigital() const {
    return ((int)dataType_ & (int)DataType::DIGITAL_TYPE) ==
           (int)DataType::DIGITAL_TYPE;
  }

  bool IsArrayType() const {
    return ((int)dataType_ & (int)DataType::ARRAY_TYPE) ==
           (int)DataType::ARRAY_TYPE;
  }

  bool IsStringType() const {
    return dataType_ == DataType::FIXCHAR || dataType_ == DataType::VARCHAR;
  }

  IDataValue(const IDataValue &dv)
      : dataType_(dv.dataType_), valType_(dv.valType_), bKey_(dv.bKey_),
        bReuse_(false) {}
  IDataValue(DataType dataType, ValueType valType, const bool bKey)
      : dataType_(dataType), valType_(valType), bKey_(bKey), bReuse_(false) {}
  virtual ~IDataValue() {}
  /**
   * return the data type for this data value
   */
  DataType GetDataType() const { return dataType_; }
  ValueType GetValueType() const { return valType_; }
  bool IsNull() { return valType_ == ValueType::NULL_VALUE; }
  bool IsKey() { return bKey_; }
  bool IsReuse() { return bReuse_; }
  void SetReuse(bool b) { bReuse_ = b; }
  // Only copy value from the dv, not include maxlength, bKey. If bMove=true,
  // array type will move byte pointer to this and dv will set to null. They are
  // maybe not same data type. All digital type will convert each other and all
  // types can be converted to string.
  virtual void Copy(IDataValue &dv, bool bMove = false) = 0;
  virtual IDataValue *Clone(bool incVal = false) = 0;
  virtual std::any GetValue() const = 0;
  virtual uint32_t WriteData(Byte *buf) const = 0;
  virtual uint32_t ReadData(Byte *buf, uint32_t len, bool bSole = true) = 0;
  virtual uint32_t WriteData(Byte *buf, bool key) const = 0;
  // Only support to save over length fileds to overflow file. So bKey_ can not
  // be true.
  virtual uint32_t WriteData(fstream &fs) const {
    assert(false);
    return 0;
  }
  // Only support to load over length fileds from overflow file. So bKey_ can
  // not be true.
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
  virtual void ToString(StrBuff &sb) const = 0;
  virtual size_t Hash() const = 0;
  virtual bool Equal(const IDataValue &dv) const = 0;
  virtual int64_t GetLong() const { // Only used for digital type
    abort();
  }
  virtual double GetDouble() const { // Only used for digital type
    abort();
  }
  // Only used for array data type
  virtual Byte *GetBuff() const { abort(); }
  virtual void Add(IDataValue &dv) {}

  friend std::ostream &operator<<(std::ostream &os, const IDataValue &dv);
  friend bool operator==(const IDataValue &dv1, const IDataValue &dv2);
  friend bool operator>(const IDataValue &dv1, const IDataValue &dv2);
  friend bool operator>=(const IDataValue &dv1, const IDataValue &dv2);
  friend bool operator<(const IDataValue &dv1, const IDataValue &dv2);
  friend bool operator<=(const IDataValue &dv1, const IDataValue &dv2);
  friend bool operator!=(const IDataValue &dv1, const IDataValue &dv2);

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
  // If true, the instance will be used in multi place, please ensure it will be
  // deleted once.
  bool bReuse_;
};

class VectorDataValue : public MVector<IDataValue *>::Type {
public:
  using vector::vector;
  ~VectorDataValue() {
    for (auto iter = begin(); iter != end(); iter++) {
      if (bDelAll_ || !(*iter)->IsReuse())
        delete *iter;
    }
  }

  void RemoveAll() {
    for (auto iter = begin(); iter != end(); iter++) {
      if (bDelAll_ || !(*iter)->IsReuse())
        delete *iter;
    }

    clear();
  }
  void SetDelAll(bool b) { bDelAll_ = b; }
  bool IsDelAll() const { return bDelAll_; }

protected:
  bool bDelAll_ = false;
};

class VectorRow : public MVector<VectorDataValue *>::Type {
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

struct DataValueHash {
  size_t operator()(const IDataValue *pDv) const { return pDv->Hash(); }
};

struct DataValueEqual {
  bool operator()(const IDataValue *ldv, const IDataValue *rdv) const {
    return (*ldv) == (*rdv);
  }
};

struct DataValueCmp {
  bool operator()(const IDataValue *ldv, const IDataValue *rdv) const {
    return (*ldv) < (*rdv);
  }
};
} // namespace storage
