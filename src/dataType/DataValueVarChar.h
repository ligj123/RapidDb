#pragma once
#include "IDataValue.h"
#include "../utils/CharsetConvert.h"

namespace storage {
  using namespace utils;
  using namespace std;

  class DataValueVarChar : public IDataValue
  {
  public:
    DataValueVarChar(ColumnBase* pColumn, bool bKey = false);
    DataValueVarChar(string val, ColumnBase* pColumn, bool bKey = false);
    DataValueVarChar(char* byArray,int len, ColumnBase* pColumn, bool bKey = false);
    ~DataValueVarChar();
  public:
    virtual std::any GetValue() const;
    virtual uint32_t WriteData(char* buf);
    virtual uint32_t ReadData(char* buf, uint32_t len = 0);
    virtual uint32_t GetLength() const;
    virtual uint32_t GetMaxLength() const;
    virtual void SetMinValue();
    virtual void SetMaxValue();
    virtual void SetDefaultValue();

    operator string() const;
    bool operator > (const DataValueVarChar& dv) const;
    bool operator < (const DataValueVarChar& dv) const;
    bool operator >= (const DataValueVarChar& dv) const;
    bool operator <= (const DataValueVarChar& dv) const;
    bool operator == (const DataValueVarChar& dv) const;
    bool operator != (const DataValueVarChar& dv) const;
    friend std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);

  protected:
    union {
      string soleValue_;
      struct {
        char* byArray_;
        uint32_t len_;
      };
    };
  };

  std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv);
}