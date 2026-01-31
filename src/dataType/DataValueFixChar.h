#pragma once
#include "../utils/BytesFuncs.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueFixChar : public IDataValue {
public:
  explicit DataValueFixChar(uint32_t maxLength = DEFAULT_MAX_FIX_LEN)
      : IDataValue(DataType::FIXCHAR, ValueType::NULL_VALUE),
        _maxLength(maxLength), _bysValue(nullptr) {}

  DataValueFixChar(const char *val, uint32_t len, uint32_t maxLength = 0)
      : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE),
        _maxLength(maxLength == 0 ? len : maxLength), _bysValue(nullptr) {
    assert(len <= _maxLength);
    _bysValue = CachePool::Apply(_maxLength);
    BytesCopy(_bysValue, val, len);
    memset(_bysValue + len, ' ', _maxLength - len);
  }

  DataValueFixChar(Byte *byArray, uint32_t maxLength)
      : IDataValue(DataType::FIXCHAR, ValueType::BYTES_VALUE),
        _bysValue(byArray), _maxLength(maxLength) {}

  DataValueFixChar(const DataValueFixChar &src) : IDataValue(src) {
    _maxLength = src._maxLength;

    if (_valType == ValueType::NULL_VALUE) {
      _bysValue = nullptr;
    } else if (_valType == ValueType::BYTES_VALUE) {
      _bysValue = src._bysValue;
    } else {
      _valType = ValueType::SOLE_VALUE;
      _bysValue = CachePool::Apply(_maxLength);
      BytesCopy(_bysValue, src._bysValue, _maxLength);
    }
  }

  DataValueFixChar(DataValueFixChar &&src) : IDataValue(std::move(src)) {
    _maxLength = src._maxLength;
    _bysValue = src._bysValue;
    src._bysValue = nullptr;
  }

  ~DataValueFixChar() {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _maxLength);
    }
  }

  DataValueFixChar &operator=(const DataValueFixChar &src) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _maxLength);
    }

    dataType_ = src.dataType_;
    _valType = src._valType;
    _refCount = 1;
    _maxLength = src._maxLength;

    if (_valType == ValueType::NULL_VALUE) {
      _bysValue = nullptr;
    } else if (_valType == ValueType::BYTES_VALUE) {
      _bysValue = src._bysValue;
    } else {
      _valType = ValueType::SOLE_VALUE;
      _bysValue = CachePool::Apply(_maxLength);
      BytesCopy(_bysValue, src._bysValue, _maxLength);
    }

    return *this;
  }

  DataValueFixChar &operator=(DataValueFixChar &&src) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _maxLength);
    }

    dataType_ = src.dataType_;
    _valType = src._valType;
    _refCount = 1;
    _maxLength = src._maxLength;
    _bysValue = src._bysValue;
    src._bysValue = nullptr;
    return *this;
  }

public:
  DataValueFixChar *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueFixChar(*this);
    } else {
      return new DataValueFixChar(_maxLength);
    }
  }

  std::any GetValue() const override {
    switch (_valType) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return string((char *)_bysValue, _maxLength);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t GetPersistenceLength(SavePosition dtPos) const override {
    if (dtPos == SavePosition::KEY) {
      return _maxLength;
    } else {
      switch (_valType) {
      case ValueType::SOLE_VALUE:
      case ValueType::BYTES_VALUE:
        return _maxLength;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
  }
  size_t Hash() const override {
    if (_valType == ValueType::NULL_VALUE)
      return 0;

    return BytesHash(_bysValue, _maxLength);
  }
  uint32_t GetDataLength() const override {
    if (_valType == ValueType::NULL_VALUE)
      return 0;
    else
      return _maxLength;
  }
  void ToString(StrBuff &sb) const override {
    if (_valType == ValueType::NULL_VALUE) {
      return;
    }

    sb.Cat((char *)_bysValue, _maxLength);
  }

  operator MString() const {
    switch (_valType) {
    case ValueType::NULL_VALUE:
    default:
      return MString("");
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return MString((char *)_bysValue, _maxLength);
    }
  }

  operator string() const {
    switch (_valType) {
    case ValueType::NULL_VALUE:
    default:
      return string("");
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return string((char *)_bysValue, _maxLength);
    }
  }
  void SetNull() override {
    if (_valType == ValueType::SOLE_VALUE)
      CachePool::Release(_bysValue, _maxLength);

    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
  }
  uint32_t GetMaxLength() const override { return _maxLength; }

  bool SetValue(string val) {
    return SetValue(val.c_str(), (uint32_t)val.size());
  }
  bool SetValue(const char *val, uint32_t len);
  bool PutValue(std::any val) override;
  bool Copy(IDataValue &dv, bool bMove = true) override;

  uint32_t WriteData(Byte *buf, SavePosition svPos) const override;
  uint32_t ReadData(const Byte *buf, uint32_t len, SavePosition svPos,
                    bool bSole = true) override;
  uint32_t WriteData(Byte *buf) const override;
  uint32_t ReadData(const Byte *buf) override;

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;

  DataValueFixChar *operator=(const char *val);
  DataValueFixChar *operator=(const MString val);
  DataValueFixChar *operator=(const string val);

  bool EQ(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this == (DataValueFixChar &)dv;
  }
  bool GT(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this > (DataValueFixChar &)dv;
  }
  bool LT(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this < (DataValueFixChar &)dv;
  }

  bool operator>(const DataValueFixChar &dv) const {
    if (_valType == ValueType::NULL_VALUE) {
      return false;
    }
    if (dv._valType == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(_bysValue, _maxLength, dv._bysValue, dv._maxLength) > 0;
  }
  bool operator<(const DataValueFixChar &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueFixChar &dv) const {
    if (_valType == ValueType::NULL_VALUE) {
      return dv._valType == ValueType::NULL_VALUE;
    }
    if (dv._valType == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(_bysValue, _maxLength, dv._bysValue, dv._maxLength) >=
           0;
  }
  bool operator<=(const DataValueFixChar &dv) const { return !(*this > dv); }
  bool operator==(const DataValueFixChar &dv) const {
    if (_valType == ValueType::NULL_VALUE) {
      return dv._valType == ValueType::NULL_VALUE;
    }
    if (dv._valType == ValueType::NULL_VALUE) {
      return false;
    }

    return BytesCompare(_bysValue, _maxLength, dv._bysValue, dv._maxLength) ==
           0;
  }
  const Byte *GetBuff() const override { return _bysValue; }
  bool operator!=(const DataValueFixChar &dv) const { return !(*this == dv); }
  friend std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);

protected:
  uint32_t _maxLength;
  Byte *_bysValue;
};

std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);
} // namespace storage
