#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueShort : public IDataValue {
public:
  DataValueShort(bool bKey = false);
  DataValueShort(int16_t val, bool bKey = false);
  DataValueShort(Byte *byArray, bool bKey = false);
  DataValueShort(const DataValueShort &src);
  DataValueShort(std::any val, bool bKey = false);
  ~DataValueShort() {}

public:
  DataValueShort *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  int64_t GetLong() const { return (int16_t) * this; }
  double GetDouble() const { return (int16_t) * this; }
  size_t Hash() const override {
    return std::hash<int16_t>{}((int16_t) * this);
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

  operator int16_t() const;
  DataValueShort &operator=(int16_t val);
  DataValueShort &operator=(const DataValueShort &src);

  bool operator>(const DataValueShort &dv) const;
  bool operator<(const DataValueShort &dv) const;
  bool operator>=(const DataValueShort &dv) const;
  bool operator<=(const DataValueShort &dv) const;
  bool operator==(const DataValueShort &dv) const;
  bool operator!=(const DataValueShort &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueShort &dv);

protected:
  union {
    int16_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueShort &dv);
} // namespace storage
