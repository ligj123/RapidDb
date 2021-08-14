#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueBool : public IDataValue {
public:
  DataValueBool(bool bKey = false);
  DataValueBool(bool val, bool bKey);
  DataValueBool(Byte *byArray, bool bKey = false);
  DataValueBool(const DataValueBool &src);
  DataValueBool(std::any val, bool bKey = false);
  ~DataValueBool() {}

public:
  DataValueBool *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  size_t Hash() const override { return std::hash<bool>{}((bool)*this); }
  bool Equal(const IDataValue &dv) const {
    if (dataType_ != dv.GetDataType())
      return false;
    return *this == (DataValueBool &)dv;
  }

  std::any GetValue() const override;
  uint32_t WriteData(Byte *buf) override { return WriteData(buf, bKey_); };
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = true) override;
  uint32_t WriteData(fstream &fs) override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetDataLength() const override {
    return bKey_ ? 1 : (valType_ == ValueType::NULL_VALUE ? 0 : 1);
  }
  uint32_t GetMaxLength() const override { return 1; }
  uint32_t GetPersistenceLength() const override {
    return bKey_ ? 1 : (valType_ == ValueType::NULL_VALUE ? 1 : 2);
  }
  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  void ToString(StrBuff &sb) const override;

  operator bool() const;
  DataValueBool &operator=(bool val);
  DataValueBool &operator=(const DataValueBool &src);

  bool operator>(const DataValueBool &dv) const;
  bool operator<(const DataValueBool &dv) const;
  bool operator>=(const DataValueBool &dv) const;
  bool operator<=(const DataValueBool &dv) const;
  bool operator==(const DataValueBool &dv) const;
  bool operator!=(const DataValueBool &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueBool &dv);

protected:
  union {
    bool soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueBool &dv);
} // namespace storage
