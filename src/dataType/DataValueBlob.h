#pragma once
#include "IDataValue.h"

namespace storage {
using namespace utils;
using namespace std;

class DataValueBlob : public IDataValue {
public:
  DataValueBlob(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueBlob(const char *val, uint32_t len,
                uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueBlob(Byte *byArray, uint32_t len,
                uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
  DataValueBlob(uint32_t maxLength, bool bKey, std::any val);
  DataValueBlob(const DataValueBlob &src);
  ~DataValueBlob();

public:
  DataValueBlob *CloneDataValue(bool incVal = false) override;
  uint32_t WriteData(Byte *buf, bool key) override;
  uint32_t GetPersistenceLength(bool key) const override;

  std::any GetValue() const override;
  uint32_t WriteData(Byte *buf) override { return WriteData(buf, bKey_); }
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = false) override;
  uint32_t WriteData(fstream &fs) override;
  uint32_t ReadData(fstream &fs) override;
  uint32_t GetDataLength() const override;
  uint32_t GetMaxLength() const override { return maxLength_; }
  uint32_t GetPersistenceLength() const override;
  void SetMinValue() override;
  void SetMaxValue() override;
  void SetDefaultValue() override;
  void ToString(StrBuff &sb) override;
  operator const char *() const;

  /**Only for byte array that first 4 bytes is lenght*/
  DataValueBlob &operator=(const char *val);
  void Put(uint32_t len, char *val);
  DataValueBlob &operator=(const DataValueBlob &src);
  bool operator==(const DataValueBlob &dv) const;
  friend std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);

protected:
  uint32_t maxLength_;
  uint32_t soleLength_;
  Byte *bysValue_;
};

std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv);
} // namespace storage
