#pragma once
#include "TDataValue.h"
#include "../utils/CharsetConvert.h"

namespace storage {
  using namespace utils;
  using namespace std;

  class DataValueVarChar : public TDataValue<string>
  {
  public:
    DataValueVarChar(int maxLen = DEFAULT_MAX_LEN, Charsets set = Charsets::UTF8);
    DataValueVarChar(string str, int maxLen = DEFAULT_MAX_LEN, Charsets set = Charsets::UTF8);
    DataValueVarChar(char* byArray, int len, int maxLen = DEFAULT_MAX_LEN, Charsets set = Charsets::UTF8);
  public:
    virtual std::any GetValue() const;
    virtual int WriteData(char* buf, bool bkey);
    virtual int ReadData(char* buf, bool bkey, int len = 0);
    virtual int GetLength(bool bKey) const;

    virtual void SetMinValue();
    virtual void SetMaxValue();
    virtual void SetDefaultValue();

    operator string() const;
    bool operator > (const DataValueVarChar& dv) const { return Lg(dv); }
    bool operator < (const DataValueVarChar& dv) const { return !Le(dv); }
    bool operator >= (const DataValueVarChar& dv) const { return Le(dv); }
    bool operator <= (const DataValueVarChar& dv) const { return !Lg(dv); }
    bool operator == (const DataValueVarChar& dv) const { return Equal(dv); }
    bool operator != (const DataValueVarChar& dv) const { return !Equal(dv); }
    friend std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);

  protected:
    Charsets charset_;
  };

  std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);
}