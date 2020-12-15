#pragma once
#include "IDataValue.h"
#include "../utils/CharsetConvert.h"
#include <any>

namespace storage {
  using namespace utils;
  using namespace std;

  class DataValueFixChar : public IDataValue
  {
  public:
    DataValueFixChar(uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false,
      utils::Charsets charset = utils::Charsets::UTF8);
    DataValueFixChar(string val, uint32_t maxLength = DEFAULT_MAX_LEN, bool bKey = false,
      utils::Charsets charset = utils::Charsets::UTF8);
    DataValueFixChar(uint32_t maxLength, char* byArray, bool bKey = false,
      utils::Charsets charset = utils::Charsets::UTF8);
    DataValueFixChar(uint32_t maxLength, bool bKey, utils::Charsets charset, std::any val);
    DataValueFixChar(const DataValueFixChar& src);
    ~DataValueFixChar();
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
    DataValueFixChar& operator=(const string& val);
    DataValueFixChar& operator=(const DataValueFixChar& src);

    bool operator > (const DataValueFixChar& dv) const;
    bool operator < (const DataValueFixChar& dv) const;
    bool operator >= (const DataValueFixChar& dv) const;
    bool operator <= (const DataValueFixChar& dv) const;
    bool operator == (const DataValueFixChar& dv) const;
    bool operator != (const DataValueFixChar& dv) const;
    friend std::ostream& operator<< (std::ostream& os, const DataValueFixChar& dv);

  protected:
    uint32_t maxLength_;
    utils::Charsets charset_;
    string soleValue_;
    char* byArray_;
  };

  std::ostream& operator<< (std::ostream& os, const DataValueFixChar& dv);
}