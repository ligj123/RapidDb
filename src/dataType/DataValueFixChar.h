#pragma once
#include "../utils/BytesFuncs.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueFixChar : public IDataValue {
public:
  DataValueFixChar(uint32_t maxLength = DEFAULT_MAX_FIX_LEN)
      : IDataValue(DataType::FIXCHAR, ValueType::NULL_VALUE),
        maxLength_(maxLength), bysValue_(nullptr) {}
  DataValueFixChar(const char *val, uint32_t len,
                   uint32_t maxLength = UINT32_MAX)
      : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE),
        maxLength_(maxLength == UINT32_MAX ? len + 1 : maxLength),
        bysValue_(nullptr) {
    assert(len + 1 <= maxLength_);
    bysValue_ = CachePool::Apply(maxLength_);
    BytesCopy(bysValue_, val, len);
    memset(bysValue_ + len, ' ', maxLength_ - len - 1);
    bysValue_[maxLength_ - 1] = 0;
  }

  DataValueFixChar(Byte *byArray, uint32_t maxLength)
      : IDataValue(DataType::FIXCHAR, ValueType::BYTES_VALUE),
        bysValue_(byArray), maxLength_(maxLength) {}

  DataValueFixChar(const DataValueFixChar &src);
  ~DataValueFixChar() {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }
  }

public:
  DataValueFixChar *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueFixChar(*this);
    } else {
      return new DataValueFixChar(maxLength_);
    }
  }

  std::any GetValue() const override {
    switch (valType_) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return MString((char *)bysValue_, maxLength_ - 1);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t GetPersistenceLength(SavePosition dtPos) const override {
    if (dtPos == SavePosition::KEY) {
      return maxLength_;
    } else {
      switch (valType_) {
      case ValueType::SOLE_VALUE:
      case ValueType::BYTES_VALUE:
        return maxLength_;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
  }
  size_t Hash() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;

    return BytesHash(bysValue_, maxLength_);
  }
  uint32_t GetDataLength() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return maxLength_;
  }
  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }

    sb.Cat((char *)bysValue_, maxLength_ - 1);
  }

  operator MString() const {
    switch (valType_) {
    case ValueType::NULL_VALUE:
    default:
      return MString("");
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return MString((char *)bysValue_);
    }
  }

  operator string() const {
    switch (valType_) {
    case ValueType::NULL_VALUE:
    default:
      return string("");
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return string((char *)bysValue_);
    }
  }
  void SetNull() override {
    if (valType_ == ValueType::SOLE_VALUE)
      CachePool::Release(bysValue_, maxLength_);

    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
  }
  uint32_t GetMaxLength() const override { return maxLength_; }

  bool SetValue(string val) {
    return SetValue(val.c_str(), (uint32_t)val.size());
  }
  bool SetValue(const char *val, uint32_t len);
  bool PutValue(std::any val) override;
  bool Copy(const IDataValue &dv, bool bMove = true) override;

  uint32_t WriteData(Byte *buf, SavePosition svPos) const override;
  uint32_t ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                    bool bSole = true) override;
  uint32_t WriteData(Byte *buf) const override;
  uint32_t ReadData(Byte *buf) override;

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;

  DataValueFixChar *operator=(const char *val);
  DataValueFixChar *operator=(const MString val);
  DataValueFixChar *operator=(const string val);
  DataValueFixChar *operator=(const DataValueFixChar &src);

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
    if (valType_ == ValueType::NULL_VALUE) {
      return false;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(bysValue_, maxLength_, dv.bysValue_, dv.maxLength_) > 0;
  }
  bool operator<(const DataValueFixChar &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueFixChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(bysValue_, maxLength_, dv.bysValue_, dv.maxLength_) >=
           0;
  }
  bool operator<=(const DataValueFixChar &dv) const { return !(*this > dv); }
  bool operator==(const DataValueFixChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return false;
    }

    return BytesCompare(bysValue_, maxLength_, dv.bysValue_, dv.maxLength_) ==
           0;
  }
  Byte *GetBuff() const override { return bysValue_; }
  bool operator!=(const DataValueFixChar &dv) const { return !(*this == dv); }
  friend std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);

protected:
  uint32_t maxLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);
} // namespace storage
