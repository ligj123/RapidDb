#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueDouble : public IDataValue {
public:
  DataValueDouble(bool bKey = false);
  DataValueDouble(double val, bool bKey = false);
  DataValueDouble(Byte *byArray, bool bKey = false);
  DataValueDouble(const DataValueDouble &src);
  DataValueDouble(std::any val, bool bKey = false);
  ~DataValueDouble() {}

public:
  DataValueDouble *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  double GetDouble() const { return (double)*this; }
  size_t Hash() const override { return std::hash<double>{}((double)*this); }
  bool Equal(const IDataValue &dv) const {
    if (!dv.IsDigital())
      return false;

    return GetDouble() == dv.GetDouble();
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

  operator double() const;
  DataValueDouble &operator=(double val);
  DataValueDouble &operator=(const DataValueDouble &src);

  bool operator>(const DataValueDouble &dv) const;
  bool operator<(const DataValueDouble &dv) const;
  bool operator>=(const DataValueDouble &dv) const;
  bool operator<=(const DataValueDouble &dv) const;
  bool operator==(const DataValueDouble &dv) const;
  bool operator!=(const DataValueDouble &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueDouble &dv);

protected:
  union {
    double soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueDouble &dv);
} // namespace storage
