#pragma once
#include "IDataValue.h"

namespace storage {
  using namespace utils;
  using namespace std;

  class DataValueBlob : public IDataValue
  {
  public:
    DataValueBlob(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueBlob(char* val, uint32_t len, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueBlob(const char* val, uint32_t len, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueBlob(Byte* byArray, uint32_t len, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueBlob(uint32_t maxLength, bool bKey, std::any val);
    DataValueBlob(const DataValueBlob& src);
    ~DataValueBlob();
  public:
    uint32_t WriteData(Byte* buf, bool key = false) override;
    uint32_t GetPersistenceLength(bool key = false) const override;

    std::any GetValue() const override;
    uint32_t WriteData(Byte* buf) override;
    uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
    uint32_t GetDataLength() const override;
    uint32_t GetMaxLength() const override;
    uint32_t GetPersistenceLength() const override;
    void SetMinValue() override;
    void SetMaxValue() override;
    void SetDefaultValue() override;

    operator const char* () const;
    /**Only for byte array that first 4 bytes is lenght*/
    DataValueBlob& operator=(const char* val);
    void Put(uint32_t len, char* val);
    DataValueBlob& operator=(const DataValueBlob& src);

    friend std::ostream& operator<< (std::ostream& os, const DataValueBlob& dv);

  protected:
    uint32_t maxLength_;
    uint32_t soleLength_;
    union
    {
      Byte* byArray_;
      char* soleValue_;
    };
  };

  std::ostream& operator<< (std::ostream& os, const DataValueBlob& dv);
}