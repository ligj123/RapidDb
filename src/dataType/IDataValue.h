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
// DataValue in record's position
enum class SavePosition : Byte {
  ALL = 0, // Data used in both key and value
  KEY,     // Data used in key
  VALUE    // Data used in value
};

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
      : dataType_(dv.dataType_), valType_(dv.valType_), savePos_(dv.savePos_),
        bReuse_(false) {}
  IDataValue(DataType dataType, ValueType valType, SavePosition savePos)
      : dataType_(dataType), valType_(valType), savePos_(savePos),
        bReuse_(false) {}
  virtual ~IDataValue() {}
  // return the data type for this data value
  inline DataType GetDataType() const { return dataType_; }
  inline ValueType GetValueType() const { return valType_; }
  inline bool IsNull() const { return valType_ == ValueType::NULL_VALUE; }
  inline SavePosition GetSavePosition() const { return savePos_; }
  inline bool IsReuse() const { return bReuse_; }
  inline void SetReuse(bool b) { bReuse_ = b; }
  // Only copy value from the dv, not include maxlength, bKey. If bMove=true,
  // array type will move byte pointer to this and source dv will set to null.
  // They are maybe not same data type. All digital type will convert each other
  // and all types can be converted to string.
  virtual bool Copy(const IDataValue &dv, bool bMove = false) = 0;
  virtual IDataValue *Clone(bool incVal = false) = 0;
  virtual std::any GetValue() const = 0;
  // Put value to this DV, if ok, return true, else set error message into
  // thread_local variable _threadErrorMsg in ErrorMsg.h
  virtual bool PutValue(std::any) = 0;
  virtual void SetNull() { abort(); };
  // If ValueType==BYTES_VALUE and SavePos==KEY, here need to consider the
  // conversion between KEY and VALUE for some DataType.
  virtual uint32_t WriteData(Byte *buf, SavePosition svPos) const = 0;
  // if bSole == true, the value will copy to new buffer for Array data type.
  // else, it will read as BYTES_VALUE value type for array data type.
  virtual uint32_t ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                            bool bSole = true) = 0;
  // Only support to save over length fileds to overflow page. So savePos_
  // can only be VALUE.
  virtual uint32_t WriteData(fstream &fs) const {
    assert(false);
    return 0;
  }
  // Only support to load over length fileds from overflow page. So savePos_ can
  // only be VALUE.
  virtual uint32_t ReadData(fstream &fs) {
    assert(false);
    return 0;
  }
  /**The memory size to save data*/
  virtual uint32_t GetDataLength() const = 0;
  /**The max memory size that can bu used to save this data*/
  virtual uint32_t GetMaxLength() const = 0;
  /**How much bytes to save this data to disk*/
  virtual uint32_t GetPersistenceLength(SavePosition dtPos) const = 0;
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
  virtual void Add(IDataValue &dv) { abort(); }

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
  SavePosition savePos_;
  // If true, the instance will be used in multi place, use it to ensure this dv
  // will be deleted once.
  bool bReuse_;
};

class VectorDataValue : public MVector<IDataValue *>::Type {
public:
  using vector::vector;

  VectorDataValue(VectorDataValue &&src) noexcept { swap(src); }

  ~VectorDataValue() {
    if (bRef) {
      clear();
      return;
    }

    for (auto iter = begin(); iter != end(); iter++) {
      if (bDelAll_ || !(*iter)->IsReuse())
        delete *iter;
    }

    clear();
  }

  VectorDataValue &operator=(VectorDataValue &&other) noexcept {
    RemoveAll();
    swap(other);
    return *this;
  }

  void RemoveAll() {
    if (bRef) {
      clear();
      return;
    }

    for (auto iter = begin(); iter != end(); iter++) {
      if (bDelAll_ || !(*iter)->IsReuse())
        delete *iter;
    }

    clear();
  }
  void SetDelAll(bool b) { bDelAll_ = b; }
  bool IsDelAll() const { return bDelAll_; }
  void SetRef(bool b) { bRef = b; }
  bool IsRef() { return bRef; }

protected:
  // If delete reused elements.
  bool bDelAll_ = false;
  // If the elements are referenced from other vector and NOT delete elements.
  bool bRef = false;
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
