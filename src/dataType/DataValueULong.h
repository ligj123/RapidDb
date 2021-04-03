#pragma once
#include "IDataValue.h"
#include <any>
#include <ostream>

namespace storage {
class DataValueULong : public IDataValue {
public:
  DataValueULong(bool bKey = false);
  DataValueULong(uint64_t val, bool bKey = false);
  DataValueULong(Byte *byArray, bool bKey = false);
  DataValueULong(const DataValueULong &src);
  DataValueULong(std::any val, bool bKey = false);
  ~DataValueULong() {}

public:
  DataValueULong *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;

  std::any GetValue() const override;
  uint32_t WriteData(Byte *buf) override;
  uint32_t ReadData(Byte *buf, uint32_t len = 0) override;
  uint32_t GetDataLength() const override;
  uint32_t GetMaxLength() const override;
  uint32_t GetPersistenceLength() const override;
  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;

  operator uint64_t() const;
  DataValueULong &operator=(uint64_t val);
  DataValueULong &operator=(const char *pArr);
  DataValueULong &operator=(const DataValueULong &src);

  bool operator>(const DataValueULong &dv) const;
  bool operator<(const DataValueULong &dv) const;
  bool operator>=(const DataValueULong &dv) const;
  bool operator<=(const DataValueULong &dv) const;
  bool operator==(const DataValueULong &dv) const;
  bool operator!=(const DataValueULong &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueULong &dv);

protected:
  union {
    uint64_t soleValue_;
    Byte *byArray_;
  };
};
std::ostream &operator<<(std::ostream &os, const DataValueULong &dv);
} // namespace storage