#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueByte : public IDataValue {
public:
  DataValueByte(bool bKey = false);
  DataValueByte(uint8_t val, bool bKey = false);
  DataValueByte(Byte *byArray, bool bKey = false);
  DataValueByte(const DataValueByte &src);
  DataValueByte(std::any val, bool bKey = false);
  ~DataValueByte() {}

public:
  DataValueByte *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  int64_t GetLong() const { return (uint8_t) * this; }
  double GetDouble() const { return (uint8_t) * this; }
  size_t Hash() const override {
    return std::hash<uint8_t>{}((uint8_t) * this);
  }
  bool Equal(const IDataValue &dv) const {
    if (!dv.IsDigital())
      return false;
    if (dv.IsAutoPrimaryKey()) {
      return GetLong() == dv.GetLong();
    } else {
      return GetDouble() == dv.GetDouble();
    }
  }

  std::any GetValue() const override;
  uint32_t WriteData(Byte *buf) override { return WriteData(buf, bKey_); }
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = true) override;
  uint32_t WriteData(fstream &fs) override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetDataLength() const override;
  uint32_t GetMaxLength() const override;
  uint32_t GetPersistenceLength() const override;
  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  void ToString(StrBuff &sb) const override;

  operator uint8_t() const;
  DataValueByte &operator=(uint8_t val);
  DataValueByte &operator=(const DataValueByte &src);

  bool operator>(const DataValueByte &dv) const;
  bool operator<(const DataValueByte &dv) const;
  bool operator>=(const DataValueByte &dv) const;
  bool operator<=(const DataValueByte &dv) const;
  bool operator==(const DataValueByte &dv) const;
  bool operator!=(const DataValueByte &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueByte &dv);

protected:
  union {
    uint8_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueByte &dv);
} // namespace storage
