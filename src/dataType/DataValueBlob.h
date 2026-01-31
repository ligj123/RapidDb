#pragma once
#include "../cache/CachePool.h"
#include "../utils/BytesFuncs.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueBlob : public IDataValue {
public:
  explicit DataValueBlob(uint32_t maxLength = DEFAULT_MAX_VAR_LEN)
      : IDataValue(DataType::BLOB, ValueType::NULL_VALUE),
        _maxLength(maxLength), _bysValue(nullptr), _soleLength(0) {}
  DataValueBlob(const char *val, int len, uint32_t maxLength = UINT32_MAX)
      : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE),
        _maxLength(maxLength == UINT32_MAX ? len : maxLength),
        _soleLength(len) {
    _bysValue = CachePool::Apply(_soleLength);
    BytesCopy(_bysValue, val, len);
  }
  DataValueBlob(int len, const Byte *val, uint32_t maxLength = UINT32_MAX)
      : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE),
        _maxLength(maxLength == UINT32_MAX ? len : maxLength),
        _soleLength(len) {
    _bysValue = CachePool::Apply(_soleLength);
    BytesCopy(_bysValue, val, len);
  }
  DataValueBlob(Byte *byArray, uint32_t len, uint32_t maxLength)
      : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE), _bysValue(byArray),
        _maxLength(maxLength), _soleLength(len) {
    assert(_soleLength <= _maxLength);
  }
  DataValueBlob(const DataValueBlob &src) : IDataValue(src) {
    _maxLength = src._maxLength;
    _soleLength = src._soleLength;

    if (_valType == ValueType::NULL_VALUE) {
      _bysValue = nullptr;
    } else {
      _valType = ValueType::SOLE_VALUE;
      _bysValue = CachePool::Apply(_soleLength);
      BytesCopy(_bysValue, src._bysValue, _soleLength);
    }
  }

  DataValueBlob(DataValueBlob &&src) : IDataValue(std::move(src)) {
    _maxLength = src._maxLength;
    _soleLength = src._soleLength;
    _bysValue = src._bysValue;
  }
  ~DataValueBlob() {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _soleLength);
      _valType = ValueType::NULL_VALUE;
    }
  }

  DataValueBlob &operator=(const DataValueBlob &src) {
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

  DataValueBlob &operator=(DataValueBlob &&src) {
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
  DataValueBlob *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueBlob(*this);
    } else {
      return new DataValueBlob(_maxLength);
    }
  }

  std::any GetValue() const override { return std::any(); }

  uint32_t GetPersistenceLength(
      SavePosition dtPos = SavePosition::VALUE) const override {
    assert(dtPos == SavePosition::VALUE);
    switch (_valType) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return _soleLength;
    case ValueType::NULL_VALUE:
    default:
      return 0;
    }
  }

  size_t Hash() const override {
    if (_valType == ValueType::NULL_VALUE)
      return 0;

    return BytesHash(_bysValue, _soleLength);
  }

  uint32_t GetDataLength() const override {
    return (_valType == ValueType::NULL_VALUE ? 0 : _soleLength);
  }

  uint32_t GetMaxLength() const override { return _maxLength; }
  void SetNull() override {
    if (_valType == ValueType::SOLE_VALUE)
      CachePool::Release(_bysValue, _soleLength);

    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
  }

  bool SetValue(vector<char> val) {
    return SetValue(val.data(), (uint32_t)val.size());
  }
  bool SetValue(const char *val, uint32_t len);
  bool PutValue(std::any val) override;
  bool Copy(IDataValue &dv, bool bMove = true) override;
  uint32_t WriteData(Byte *buf, SavePosition dtPos) const override;
  uint32_t ReadData(const Byte *buf, uint32_t len, SavePosition dtPos,
                    bool bSole = false) override;
  uint32_t WriteData(Byte *buf) const override;
  uint32_t ReadData(const Byte *buf) override;

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  operator const char *() const;
  char *Get(uint32_t &len) {
    len = _soleLength;
    return (char *)_bysValue;
  }

  bool operator==(const DataValueBlob &dv) const;
  const Byte *GetBuff() const override { return _bysValue; }
  void ToString(StrBuff &sb) const override;

  bool EQ(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this == (DataValueBlob &)dv;
  }

  bool GT(const IDataValue &dv) const override { abort(); }
  bool LT(const IDataValue &dv) const override { abort(); }

  friend std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);

protected:
  uint32_t _maxLength;
  uint32_t _soleLength;
  Byte *_bysValue;
};

std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);
} // namespace storage
