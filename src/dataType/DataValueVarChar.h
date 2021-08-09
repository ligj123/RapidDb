#pragma once
#include "IDataValue.h"

namespace storage {
using namespace utils;
using namespace std;

class DataValueVarChar : public IDataValue {
public:
  DataValueVarChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueVarChar(const char *val, uint32_t strLen,
                   uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueVarChar(Byte *byArray, uint32_t strLen,
                   uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueVarChar(uint32_t maxLength, bool bKey, std::any val);
  DataValueVarChar(const DataValueVarChar &src);
  ~DataValueVarChar();

public:
  DataValueVarChar *CloneDataValue(bool incVal = false) override;
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

  operator string() const;
  DataValueVarChar &operator=(char *val);
  DataValueVarChar &operator=(const char *val);
  DataValueVarChar &operator=(const DataValueVarChar &src);

  bool operator>(const DataValueVarChar &dv) const;
  bool operator<(const DataValueVarChar &dv) const;
  bool operator>=(const DataValueVarChar &dv) const;
  bool operator<=(const DataValueVarChar &dv) const;
  bool operator==(const DataValueVarChar &dv) const;
  bool operator!=(const DataValueVarChar &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv);

protected:
  uint32_t maxLength_;
  uint32_t soleLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv);
} // namespace storage
