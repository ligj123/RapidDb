#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueInt : public IDataValue {
public:
  DataValueInt(bool bKey = false);
  DataValueInt(int32_t val, bool bKey = false);
  DataValueInt(Byte *byArray, bool bKey = false);
  DataValueInt(const DataValueInt &src);
  DataValueInt(std::any val, bool bKey = false);
  ~DataValueInt() {}

public:
  DataValueInt *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  int64_t GetLong() const { return (int32_t) * this; }
  double GetDouble() const { return (int32_t) * this; }
  size_t Hash() const override {
    return std::hash<int32_t>{}((int32_t) * this);
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

  operator int32_t() const;
  DataValueInt &operator=(int32_t val);
  DataValueInt &operator=(const DataValueInt &src);

  bool operator>(const DataValueInt &dv) const;
  bool operator<(const DataValueInt &dv) const;
  bool operator>=(const DataValueInt &dv) const;
  bool operator<=(const DataValueInt &dv) const;
  bool operator==(const DataValueInt &dv) const;
  bool operator!=(const DataValueInt &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueInt &dv);

protected:
  union {
    int32_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueInt &dv);
} // namespace storage
