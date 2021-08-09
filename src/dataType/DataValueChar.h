﻿#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueChar : public IDataValue {
public:
  DataValueChar(bool bKey = false);
  DataValueChar(int8_t val, bool bKey = false);
  DataValueChar(Byte *byArray, bool bKey = false);
  DataValueChar(const DataValueChar &src);
  DataValueChar(std::any val, bool bKey = false);
  ~DataValueChar() {}

public:
  DataValueChar *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;

  std::any GetValue() const override;
  uint32_t WriteData(Byte *buf) override;
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = true) override;
  uint32_t WriteData(fstream &fs) override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetDataLength() const override;
  uint32_t GetMaxLength() const override;
  uint32_t GetPersistenceLength() const override;
  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  void ToString(StrBuff &sb) override;

  operator int8_t() const;
  DataValueChar &operator=(int8_t val);
  DataValueChar &operator=(const DataValueChar &src);

  bool operator>(const DataValueChar &dv) const;
  bool operator<(const DataValueChar &dv) const;
  bool operator>=(const DataValueChar &dv) const;
  bool operator<=(const DataValueChar &dv) const;
  bool operator==(const DataValueChar &dv) const;
  bool operator!=(const DataValueChar &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueChar &dv);

protected:
  union {
    int8_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueChar &dv);
} // namespace storage
