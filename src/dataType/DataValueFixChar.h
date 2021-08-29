#pragma once
#include "IDataValue.h"
#include <any>

namespace storage {
using namespace utils;
using namespace std;

class DataValueFixChar : public IDataValue {
public:
  DataValueFixChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueFixChar(const char *val, uint32_t strLen,
                   uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueFixChar(Byte *byArray, uint32_t maxLength, bool bKey = false);
  DataValueFixChar(uint32_t maxLength, bool bKey, std::any val);
  DataValueFixChar(const DataValueFixChar &src);
  ~DataValueFixChar();

public:
  void Copy(IDataValue &dv, bool bMove = true) override;
  DataValueFixChar *Clone(bool incVal = false) override {
    if (incVal) {
      return new DataValueFixChar(*this);
    } else {
      return new DataValueFixChar(maxLength_, bKey_);
    }
  }
  uint32_t WriteData(Byte *buf) const override {
    return WriteData(buf, bKey_);
  };
  uint32_t WriteData(Byte *buf, bool key) const override;
  uint32_t GetPersistenceLength() const override {
    return GetPersistenceLength(bKey_);
  }
  uint32_t GetPersistenceLength(bool key) const override {
    if (key) {
      return maxLength_;
    } else {
      switch (valType_) {
      case ValueType::SOLE_VALUE:
        return maxLength_ + 1;
      case ValueType::NULL_VALUE:
      default:
        return 1;
      }
    }
  }
  size_t Hash() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    size_t h = 0;
    for (uint32_t i = 0; i < maxLength_; i++) {
      h = (h << 1) ^ bysValue_[i];
    }
    return h;
  }
  bool Equal(const IDataValue &dv) const override {
    if (dataType_ != dv.GetDataType())
      return false;
    return *this == (DataValueFixChar &)dv;
  }

  std::any GetValue() const override;
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = true) override;
  uint32_t WriteData(fstream &fs) const override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetDataLength() const override {
    if (!bKey_ && valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return maxLength_;
  }
  uint32_t GetMaxLength() const override { return maxLength_; }

  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }

    sb.Cat((char *)bysValue_, maxLength_ - 1);
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

  DataValueFixChar &operator=(const char *val);
  DataValueFixChar &operator=(const DataValueFixChar &src);

  bool operator>(const DataValueFixChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return false;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return strcmp((char *)bysValue_, (char *)dv.bysValue_);
  }
  bool operator<(const DataValueFixChar &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueFixChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return strcmp((char *)bysValue_, (char *)dv.bysValue_);
  }
  bool operator<=(const DataValueFixChar &dv) const { return !(*this > dv); }
  bool operator==(const DataValueFixChar &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return false;
    }

    return strcmp((char *)bysValue_, (char *)dv.bysValue_) == 0;
  }
  Byte *GetBuff() const override { return bysValue_; }
  bool operator!=(const DataValueFixChar &dv) const { return !(*this == dv); }
  friend std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);

protected:
  uint32_t maxLength_;
  // uint32_t soleLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);
} // namespace storage
