#pragma once
#include "../utils/BytesConvert.h"
#include "IDataValue.h"

namespace storage {
using namespace std;

class DataValueVarChar : public IDataValue {
public:
  DataValueVarChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false)
      : IDataValue(DataType::VARCHAR, ValueType::NULL_VALUE, bKey),
        maxLength_(maxLength), soleLength_(0), bysValue_(nullptr) {}
  DataValueVarChar(const char *val, uint32_t strLen,
                   uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false)
      : IDataValue(DataType::VARCHAR, ValueType::SOLE_VALUE, bKey),
        maxLength_(maxLength), soleLength_(strLen + 1) {
    if (soleLength_ >= maxLength_) {
      throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {to_string(maxLength_), to_string(soleLength_)});
    }

    bysValue_ = CachePool::Apply(soleLength_);
    BytesCopy(bysValue_, val, soleLength_);
  }

  DataValueVarChar(uint32_t maxLength, bool bKey, std::any val);
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
  void Copy(const IDataValue &dv, bool bMove = true) override;
  DataValueVarChar *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueVarChar(*this);
    } else {
      return new DataValueVarChar(maxLength_, bKey_);
    }
  }
  uint32_t GetPersistenceLength() const override {
    return GetPersistenceLength(bKey_);
  }
  uint32_t GetPersistenceLength(bool key) const override {
    if (key) {
      if (valType_ == ValueType::NULL_VALUE) {
        return 1;
      }
      return soleLength_;
    } else {
      switch (valType_) {
      case ValueType::SOLE_VALUE:
        return soleLength_ + 1 + sizeof(uint32_t);
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
  std::any GetValue() const override {
    switch (valType_) {
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return MString((char *)bysValue_);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t WriteData(Byte *buf) const override { return WriteData(buf, bKey_); }
  uint32_t WriteData(Byte *buf, bool key) const override;
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = true) override;
  uint32_t WriteData(fstream &fs) const override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetDataLength() const override {
    if (bKey_) {
      if (valType_ == ValueType::NULL_VALUE)
        return 1;
      else
        return soleLength_;
    } else {
      if (valType_ == ValueType::NULL_VALUE)
        return 0;
      else
        return soleLength_;
    }
  }
  uint32_t GetMaxLength() const override { return maxLength_; }
  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }

    sb.Cat((char *)bysValue_, soleLength_ - 1);
  }

  operator MString() const {
    switch (valType_) {
    case ValueType::NULL_VALUE:
      return "";
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return MString((char *)bysValue_);
    }

    return "";
  }

  operator string() const {
    switch (valType_) {
    case ValueType::NULL_VALUE:
      return "";
    case ValueType::SOLE_VALUE:
    case ValueType::BYTES_VALUE:
      return string((char *)bysValue_);
    }

    return "";
  }

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

    return strcmp((char *)bysValue_, (char *)dv.bysValue_);
  }
  bool operator<(const DataValueVarChar &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueVarChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return strcmp((char *)bysValue_, (char *)dv.bysValue_);
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

    return strcmp((char *)bysValue_, (char *)dv.bysValue_) == 0;
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
