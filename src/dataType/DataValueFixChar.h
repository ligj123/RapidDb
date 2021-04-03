#pragma once
#include "IDataValue.h"
#include <any>

namespace storage {
using namespace utils;
using namespace std;

class DataValueFixChar : public IDataValue {
public:
  DataValueFixChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueFixChar(char *val, uint32_t maxLength = DEFAULT_MAX_LEN,
                   bool bKey = false);
  DataValueFixChar(const char *val, uint32_t maxLength = DEFAULT_MAX_LEN,
                   bool bKey = false);
  DataValueFixChar(Byte *byArray, uint32_t strLen, uint32_t maxLength,
                   bool bKey = false);
  DataValueFixChar(uint32_t maxLength, bool bKey, std::any val);
  DataValueFixChar(const DataValueFixChar &src);
  ~DataValueFixChar();

public:
  DataValueFixChar *CloneDataValue(bool incVal = false) override;
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

  operator string() const;
  DataValueFixChar &operator=(char *val);
  DataValueFixChar &operator=(const char *val);
  DataValueFixChar &operator=(const DataValueFixChar &src);

  bool operator>(const DataValueFixChar &dv) const;
  bool operator<(const DataValueFixChar &dv) const;
  bool operator>=(const DataValueFixChar &dv) const;
  bool operator<=(const DataValueFixChar &dv) const;
  bool operator==(const DataValueFixChar &dv) const;
  bool operator!=(const DataValueFixChar &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);

protected:
  uint32_t maxLength_;
  uint32_t soleLength_;
  union {
    char *soleValue_;
    Byte *byArray_;
  };
};

std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv);
} // namespace storage