#pragma once
#include "../cache/CachePool.h"
#include "../utils/BytesConvert.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueBlob : public IDataValue {
public:
  DataValueBlob(uint32_t maxLength = DEFAULT_MAX_VAR_LEN)
      : IDataValue(DataType::BLOB, ValueType::NULL_VALUE, SavePosition::VALUE),
        maxLength_(maxLength), bysValue_(nullptr), soleLength_(0) {}
  DataValueBlob(const char *val, int len, uint32_t maxLength = UINT32_MAX)
      : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, SavePosition::VALUE),
        maxLength_(maxLength == UINT32_MAX ? len : maxLength),
        soleLength_(len) {
    bysValue_ = CachePool::Apply(soleLength_);
    BytesCopy(bysValue_, val, len);
  }
  DataValueBlob(Byte *byArray, uint32_t len, uint32_t maxLength,
                SavePosition svPos = SavePosition::VALUE)
      : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, svPos),
        bysValue_(byArray), maxLength_(maxLength), soleLength_(len) {
    assert(soleLength_ <= maxLength_);
  }
  DataValueBlob(const DataValueBlob &src) : IDataValue(src) {
    maxLength_ = src.maxLength_;
    soleLength_ = src.soleLength_;

    switch (valType_) {
    case ValueType::SOLE_VALUE:
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, src.bysValue_, soleLength_);
      break;
    case ValueType::BYTES_VALUE:
      bysValue_ = src.bysValue_;
      break;
    case ValueType::NULL_VALUE:
    default:
      bysValue_ = nullptr;
      break;
    }
  }
  ~DataValueBlob() {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, soleLength_);
      valType_ = ValueType::NULL_VALUE;
    }
  }

public:
  DataValueBlob *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueBlob(*this);
    } else {
      return new DataValueBlob(maxLength_);
    }
  }

  std::any GetValue() const override { return std::any(); }

  uint32_t GetPersistenceLength(SavePosition dtPos) const override {
    if (dtPos == SavePosition::KEY_FIX || dtPos == SavePosition::KEY_VAR) {
      return maxLength_;
    } else {
      switch (valType_) {
      case ValueType::SOLE_VALUE:
      case ValueType::BYTES_VALUE:
        return soleLength_;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
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

  uint32_t GetDataLength() const override {
    return (valType_ == ValueType::NULL_VALUE ? 0 : soleLength_);
  }

  uint32_t GetMaxLength() const override { return maxLength_; }
  void SetNull() override {
    if (valType_ == ValueType::SOLE_VALUE)
      CachePool::Release(bysValue_, soleLength_);

    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
  }

  bool SetValue(vector<char> val) {
    return SetValue(val.data(), (uint32_t)val.size());
  }
  bool SetValue(const char *val, uint32_t len);
  bool PutValue(std::any val) override;
  bool Copy(const IDataValue &dv, bool bMove = true) override;
  uint32_t WriteData(Byte *buf, SavePosition dtPos) const override;
  uint32_t ReadData(Byte *buf, uint32_t len, SavePosition dtPos,
                    bool bSole = false) override;
  uint32_t WriteData(Byte *buf) const override;
  uint32_t ReadData(Byte *buf) override;

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  operator const char *() const;
  char *Get(uint32_t &len) {
    len = soleLength_;
    return (char *)bysValue_;
  }
  DataValueBlob &operator=(const DataValueBlob &src);
  bool operator==(const DataValueBlob &dv) const;
  Byte *GetBuff() const override { return bysValue_; }
  void ToString(StrBuff &sb) const override;

  bool EQ(const IDataValue &dv) const override {
    assert(dataType_ == dv.GetDataType());
    return *this == (DataValueBlob &)dv;
  }

  bool GT(const IDataValue &dv) const override { abort(); }
  bool LT(const IDataValue &dv) const override { abort(); }

  friend std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);

protected:
  uint32_t maxLength_;
  uint32_t soleLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);
} // namespace storage
