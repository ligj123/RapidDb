#pragma once
#include "../cache/CachePool.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueBlob : public IDataValue {
public:
  DataValueBlob(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueBlob(const char *val, uint32_t len,
                uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueBlob(Byte *byArray, uint32_t len,
                uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueBlob(MVector<Byte>::Type &vct, uint32_t maxLength = DEFAULT_MAX_LEN,
                bool bKey = false);
  DataValueBlob(const DataValueBlob &src);
  ~DataValueBlob();

public:
  void Copy(const IDataValue &dv, bool bMove = true) override;

  DataValueBlob *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueBlob(*this);
    } else {
      return new DataValueBlob(maxLength_, bKey_);
    }
  }
  uint32_t WriteData(Byte *buf) const override { return WriteData(buf, bKey_); }
  uint32_t WriteData(Byte *buf, bool key) const override;
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = false) override;
  uint32_t WriteData(fstream &fs) const override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetPersistenceLength() const override {
    return GetPersistenceLength(bKey_);
  }
  uint32_t GetPersistenceLength(bool key) const override {
    assert(!bKey_);
    switch (valType_) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return soleLength_;
    case ValueType::NULL_VALUE:
    default:
      return 0;
    }
  }
  size_t Hash() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    size_t h = 0;
    for (uint32_t i = 0; i < soleLength_; i++) {
      h = (h << 1) ^ bysValue_[i];
    }
    return h;
  }
  bool Equal(const IDataValue &dv) const override {
    if (dataType_ != dv.GetDataType())
      return false;

    return *this == (DataValueBlob &)dv;
  }

  std::any GetValue() const override {
    switch (valType_) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return (char *)bysValue_;
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t GetDataLength() const override {
    assert(!bKey_);
    return (valType_ == ValueType::NULL_VALUE ? 0 : soleLength_);
  }
  uint32_t GetMaxLength() const override { return maxLength_; }

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  operator const char *() const;
  void Put(uint32_t len, char *val);
  char *Get(uint32_t &len) {
    len = soleLength_;
    return (char *)bysValue_;
  }
  DataValueBlob &operator=(const DataValueBlob &src);
  bool operator==(const DataValueBlob &dv) const;
  Byte *GetBuff() const override { return bysValue_; }
  void ToString(StrBuff &sb) const override;
  friend std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);

protected:
  uint32_t maxLength_;
  uint32_t soleLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);
} // namespace storage
