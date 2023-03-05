#pragma once
#include "../utils/BytesConvert.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueVarChar : public IDataValue {
public:
  DataValueVarChar(uint32_t maxLength = DEFAULT_MAX_VAR_LEN)
      : IDataValue(DataType::VARCHAR, ValueType::NULL_VALUE, SavePosition::ALL),
        maxLength_(maxLength), soleLength_(0), bysValue_(nullptr) {}
  DataValueVarChar(Byte *byArray, uint32_t strLen, uint32_t maxLength,
                   SavePosition svPos)
      : IDataValue(DataType::VARCHAR, ValueType::BYTES_VALUE, svPos),
        maxLength_(maxLength), soleLength_(strLen), bysValue_(byArray) {
    assert(soleLength_ >= maxLength_);
  }

  DataValueVarChar(const DataValueVarChar &src) : IDataValue(src) {
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

  ~DataValueVarChar() {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release((Byte *)bysValue_, soleLength_);
      valType_ = ValueType::NULL_VALUE;
    }
  }

public:
  DataValueVarChar *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueVarChar(*this);
    } else {
      return new DataValueVarChar(maxLength_);
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
      if (valType_ == ValueType::NULL_VALUE) {
        return 1;
      }
      return soleLength_;
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
  bool Equal(const IDataValue &dv) const override {
    if (dataType_ != dv.GetDataType())
      return false;
    return *this == (DataValueVarChar &)dv;
  }
  uint32_t GetDataLength() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return soleLength_;
  }
  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }

    sb.Cat((char *)bysValue_, soleLength_ - 1);
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
  uint32_t GetMaxLength() const override { return maxLength_; }
  void SetNull() override {
    if (valType_ == ValueType::SOLE_VALUE)
      CachePool::Release(bysValue_, soleLength_);

    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
  }

  bool SetValue(string val) { return SetValue(val.c_str(), (int)val.size()); }
  bool SetValue(const char *val, int len);
  bool PutValue(std::any val);
  bool Copy(const IDataValue &dv, bool bMove = true) override;

  uint32_t WriteData(Byte *buf, SavePosition svPos) const override;
  uint32_t ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                    bool bSole = true) override;
  uint32_t WriteData(Byte *buf) const override;
  uint32_t ReadData(Byte *buf) override;

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;

  DataValueVarChar &operator=(const char *val);
  DataValueVarChar &operator=(const MString val);
  DataValueVarChar &operator=(const string val);
  DataValueVarChar &operator=(const DataValueVarChar &src);

  bool operator>(const DataValueVarChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return false;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(bysValue_, soleLength_, dv.bysValue_, dv.soleLength_) >
           0;
  }
  bool operator<(const DataValueVarChar &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueVarChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return BytesCompare(bysValue_, soleLength_, dv.bysValue_, dv.soleLength_) >=
           0;
  }
  bool operator<=(const DataValueVarChar &dv) const { return !(*this > dv); }
  bool operator==(const DataValueVarChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return false;
    }

    if (GetDataLength() != dv.GetDataLength())
      return false;

    return BytesCompare(bysValue_, soleLength_, dv.bysValue_, dv.soleLength_) ==
           0;
  }
  bool operator!=(const DataValueVarChar &dv) const { return !(*this == dv); }
  Byte *GetBuff() const override { return bysValue_; }
  friend std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv);

protected:
  uint32_t maxLength_;
  uint32_t soleLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv);
} // namespace storage
