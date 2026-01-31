#pragma once
#include "../utils/BytesFuncs.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueVarChar : public IDataValue {
public:
  explicit DataValueVarChar(uint32_t maxLength = DEFAULT_MAX_VAR_LEN)
      : IDataValue(DataType::VARCHAR, ValueType::NULL_VALUE),
        _maxLength(maxLength), _soleLength(0), _bysValue(nullptr) {}
  DataValueVarChar(const char *val, uint32_t len,
                   uint32_t maxLength = UINT32_MAX)
      : IDataValue(DataType::VARCHAR, ValueType::SOLE_VALUE),
        _maxLength(maxLength == UINT32_MAX ? len : maxLength),
        _soleLength(len) {
    _bysValue = CachePool::Apply(_soleLength);
    BytesCopy(_bysValue, val, len);
  }
  DataValueVarChar(Byte *byArray, uint32_t strLen, uint32_t maxLength)
      : IDataValue(DataType::VARCHAR, ValueType::BYTES_VALUE),
        _maxLength(maxLength), _soleLength(strLen), _bysValue(byArray) {
    assert(_soleLength <= _maxLength);
  }
  DataValueVarChar(uint32_t strLen, const char *byConstCast)
      : IDataValue(DataType::VARCHAR, ValueType::BYTES_VALUE),
        _maxLength(strLen), _soleLength(strLen),
        _bysValue(reinterpret_cast<Byte *>(const_cast<char *>(byConstCast))) {
    assert(_soleLength <= _maxLength);
  }

  DataValueVarChar(const DataValueVarChar &src) : IDataValue(src) {
    _maxLength = src._maxLength;
    _soleLength = src._soleLength;

    if (_valType == ValueType::NULL_VALUE) {
      _bysValue = nullptr;
    } else if (_valType == ValueType::BYTES_VALUE) {
      _bysValue = src._bysValue;
    } else {
      _valType = ValueType::SOLE_VALUE;
      _bysValue = CachePool::Apply(_soleLength);
      BytesCopy(_bysValue, src._bysValue, _soleLength);
    }
  }

  DataValueVarChar(DataValueVarChar &&src) : IDataValue(move(src)) {
    _maxLength = src._maxLength;
    _soleLength = src._soleLength;
    _bysValue = src._bysValue;
    src._bysValue = nullptr;
  }

  ~DataValueVarChar() {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release((Byte *)_bysValue, _soleLength);
      _valType = ValueType::NULL_VALUE;
    }
  }

  DataValueVarChar &operator=(const DataValueVarChar &src) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _soleLength);
    }

    dataType_ = src.dataType_;
    _valType = src._valType;
    _refCount = 1;
    _maxLength = src._maxLength;
    _soleLength = src._soleLength;

    if (_valType == ValueType::NULL_VALUE) {
      _bysValue = nullptr;
    } else if (_valType == ValueType::BYTES_VALUE) {
      _bysValue = src._bysValue;
    } else {
      _valType = ValueType::SOLE_VALUE;
      _bysValue = CachePool::Apply(_soleLength);
      BytesCopy(_bysValue, src._bysValue, _soleLength);
    }

    return *this;
  }

  DataValueVarChar &operator=(DataValueVarChar &&src) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _soleLength);
    }

    dataType_ = src.dataType_;
    _valType = src._valType;
    _refCount = 1;
    _maxLength = src._maxLength;
    _soleLength = src._soleLength;
    _bysValue = src._bysValue;
    src._bysValue = nullptr;
    src._valType = ValueType::NULL_VALUE;
    return *this;
  }

public:
  DataValueVarChar *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueVarChar(*this);
    } else {
      return new DataValueVarChar(_maxLength);
    }
  }

  std::any GetValue() const override {
    switch (_valType) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return string((char *)_bysValue, _soleLength);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t GetPersistenceLength(SavePosition dtPos) const override {
    if (dtPos == SavePosition::KEY) {
      if (_valType == ValueType::NULL_VALUE) {
        return 1;
      }
      return _soleLength < _maxLength ? _soleLength + 1 : _maxLength;
    } else {
      switch (_valType) {
      case ValueType::SOLE_VALUE:
      case ValueType::BYTES_VALUE:
        return _soleLength;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
  }
  size_t Hash() const override {
    if (_valType == ValueType::NULL_VALUE)
      return 0;

    return BytesHash(_bysValue, _soleLength);
  }
  uint32_t GetDataLength() const override {
    if (_valType == ValueType::NULL_VALUE)
      return 0;
    else
      return _soleLength;
  }
  void ToString(StrBuff &sb) const override {
    if (_valType == ValueType::NULL_VALUE) {
      return;
    }

    sb.Cat((char *)_bysValue, _soleLength);
  }
  operator MString() const {
    switch (_valType) {
    case ValueType::NULL_VALUE:
    default:
      return MString("");
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return MString((char *)_bysValue, _soleLength);
    }
  }

  operator string() const {
    switch (_valType) {
    case ValueType::NULL_VALUE:
    default:
      return string("");
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return string((char *)_bysValue, _soleLength);
    }
  }
  uint32_t GetMaxLength() const override { return _maxLength; }
  void SetNull() override {
    if (_valType == ValueType::SOLE_VALUE)
      CachePool::Release(_bysValue, _soleLength);

    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
  }

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

  DataValueVarChar &operator=(const char *val);
  DataValueVarChar &operator=(const MString val);
  DataValueVarChar &operator=(const string val);

  bool EQ(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this == (DataValueVarChar &)dv;
  }
  bool GT(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this > (DataValueVarChar &)dv;
  }
  bool LT(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this < (DataValueVarChar &)dv;
  }

  bool operator>(const DataValueVarChar &dv) const {
    if (_valType == ValueType::NULL_VALUE) {
      return false;
    }
    if (dv._valType == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(_bysValue, _soleLength, dv._bysValue, dv._soleLength) >
           0;
  }
  bool operator<(const DataValueVarChar &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueVarChar &dv) const {
    if (_valType == ValueType::NULL_VALUE) {
      return dv._valType == ValueType::NULL_VALUE;
    }
    if (dv._valType == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(_bysValue, _soleLength, dv._bysValue, dv._soleLength) >=
           0;
  }
  bool operator<=(const DataValueVarChar &dv) const { return !(*this > dv); }
  bool operator==(const DataValueVarChar &dv) const {
    if (_valType == ValueType::NULL_VALUE) {
      return dv._valType == ValueType::NULL_VALUE;
    }
    if (dv._valType == ValueType::NULL_VALUE) {
      return false;
    }

    if (GetDataLength() != dv.GetDataLength())
      return false;

    return BytesCompare(_bysValue, _soleLength, dv._bysValue, dv._soleLength) ==
           0;
  }
  bool operator!=(const DataValueVarChar &dv) const { return !(*this == dv); }
  const Byte *GetBuff() const override { return _bysValue; }
  friend std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv);

protected:
  uint32_t _maxLength;
  uint32_t _soleLength;
  Byte *_bysValue;
};

std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv);
} // namespace storage
