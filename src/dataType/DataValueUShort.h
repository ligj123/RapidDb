#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueUShort : public IDataValue {
public:
  DataValueUShort(bool bKey = false);
  DataValueUShort(uint16_t val, bool bKey = false);
  DataValueUShort(Byte *byArray, bool bKey = false);
  DataValueUShort(const DataValueUShort &src);
  DataValueUShort(std::any val, bool bKey = false);
  ~DataValueUShort() {}

public:
  DataValueUShort *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  int64_t GetLong() const { return (uint16_t) * this; }
  double GetDouble() const { return (uint16_t) * this; }
  size_t Hash() const override {
    return std::hash<uint16_t>{}((uint16_t) * this);
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

  operator uint16_t() const;
  DataValueUShort &operator=(uint16_t val);
  DataValueUShort &operator=(const DataValueUShort &src);

  bool operator>(const DataValueUShort &dv) const;
  bool operator<(const DataValueUShort &dv) const;
  bool operator>=(const DataValueUShort &dv) const;
  bool operator<=(const DataValueUShort &dv) const;
  bool operator==(const DataValueUShort &dv) const;
  bool operator!=(const DataValueUShort &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueUShort &dv);

protected:
  union {
    uint16_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueUShort &dv);
} // namespace storage
