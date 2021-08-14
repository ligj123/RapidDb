#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueFloat : public IDataValue {
public:
  DataValueFloat(bool bKey = false);
  DataValueFloat(float val, bool bKey = false);
  DataValueFloat(Byte *byArray, bool bKey = false);
  DataValueFloat(const DataValueFloat &src);
  DataValueFloat(std::any val, bool bKey = false);
  ~DataValueFloat() {}

public:
  DataValueFloat *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;
  double GetDouble() const { return (float)*this; }
  size_t Hash() const override { return std::hash<float>{}((float)*this); }
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

  operator float() const;
  DataValueFloat &operator=(float val);
  DataValueFloat &operator=(const DataValueFloat &src);

  bool operator>(const DataValueFloat &dv) const;
  bool operator<(const DataValueFloat &dv) const;
  bool operator>=(const DataValueFloat &dv) const;
  bool operator<=(const DataValueFloat &dv) const;
  bool operator==(const DataValueFloat &dv) const;
  bool operator!=(const DataValueFloat &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueFloat &dv);

protected:
  union {
    float soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueFloat &dv);
} // namespace storage
