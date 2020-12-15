#pragma once
#include "IDataValue.h"
#include "../utils/CharsetConvert.h"

namespace storage {
  using namespace utils;
  using namespace std;

  class DataValueVarChar : public IDataValue
  {
  public:
    DataValueVarChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false, utils::Charsets charset = utils::Charsets::UTF8);
    DataValueVarChar(string val, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false, utils::Charsets charset = utils::Charsets::UTF8);
    DataValueVarChar(char* byArray, uint32_t len, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false, utils::Charsets charset = utils::Charsets::UTF8);
    DataValueVarChar(uint32_t maxLength, bool bKey, utils::Charsets charset, std::any val);
    DataValueVarChar(const DataValueVarChar& src);
    ~DataValueVarChar();
  public:
    virtual std::any GetValue() const;
    virtual uint32_t WriteData(char* buf);
    virtual uint32_t ReadData(char* buf, uint32_t len = 0);
    virtual uint32_t GetLength() const;
    virtual uint32_t GetMaxLength() const;
    virtual uint32_t GetPersistenceLength() const;
    virtual void SetMinValue();
    virtual void SetMaxValue();
    virtual void SetDefaultValue();

    operator string() const;
    DataValueVarChar& operator=(const string& val);
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
    utils::Charsets charset_;
    string soleValue_;
    struct {
      char* byArray_;
      uint32_t len_;
    };
  };

  std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);
}