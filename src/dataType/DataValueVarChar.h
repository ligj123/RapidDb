#pragma once
#include "IDataValue.h"
#include "../utils/CharsetConvert.h"

namespace storage {
  using namespace utils;
  using namespace std;

  class DataValueVarChar : public IDataValue
  {
  public:
    DataValueVarChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueVarChar(char* val, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueVarChar(Byte* byArray, uint32_t strlen, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false);
    DataValueVarChar(uint32_t maxLength, bool bKey, std::any val);
    DataValueVarChar(const DataValueVarChar& src);
    ~DataValueVarChar();
  public:
    std::any GetValue() const override;
    uint32_t WriteData(Byte* buf) override;
    uint32_t ReadData(Byte* buf, uint32_t len = 0) override;
    uint32_t GetLength() const override;
    uint32_t GetMaxLength() const override;
    uint32_t GetPersistenceLength() const override;
    void SetMinValue() override;
    void SetMaxValue() override;
    void SetDefaultValue() override;

    operator string() const;
    operator char* () const;
    DataValueVarChar& operator=(char* val);
    DataValueVarChar& operator=(const DataValueVarChar& src);

    bool operator > (const DataValueVarChar& dv) const;
    bool operator < (const DataValueVarChar& dv) const;
    bool operator >= (const DataValueVarChar& dv) const;
    bool operator <= (const DataValueVarChar& dv) const;
    bool operator == (const DataValueVarChar& dv) const;
    bool operator != (const DataValueVarChar& dv) const;
    friend std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);

  protected:
    uint32_t maxLength_;
    uint32_t soleLength_;
    struct {
      Byte* byArray_;
      char* soleValue_;
    };
  };

  std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);
}