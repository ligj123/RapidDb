﻿#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueUInt : public IDataValue {
public:
  DataValueUInt(bool bKey = false);
  DataValueUInt(uint32_t val, bool bKey = false);
  DataValueUInt(Byte *byArray, bool bKey = false);
  DataValueUInt(const DataValueUInt &src);
  DataValueUInt(std::any val, bool bKey = false);
  ~DataValueUInt() {}

public:
  DataValueUInt *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  int64_t GetLong() const { return (uint32_t) * this; }
  double GetDouble() const { return (uint32_t) * this; }
  size_t Hash() const override {
    return std::hash<uint32_t>{}((uint32_t) * this);
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

  operator uint32_t() const;
  DataValueUInt &operator=(uint32_t val);
  DataValueUInt &operator=(const DataValueUInt &src);

  bool operator>(const DataValueUInt &dv) const;
  bool operator<(const DataValueUInt &dv) const;
  bool operator>=(const DataValueUInt &dv) const;
  bool operator<=(const DataValueUInt &dv) const;
  bool operator==(const DataValueUInt &dv) const;
  bool operator!=(const DataValueUInt &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueUInt &dv);

protected:
  union {
    uint32_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueUInt &dv);
} // namespace storage
