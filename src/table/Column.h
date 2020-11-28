#pragma once
#include <string>
#include <any>
#include "../dataType/DataType.h"
#include "../utils/CharsetConvert.h"

namespace storage {
  class IDataValue;
  using namespace std;
  class ColumnBase {
  public:
    ColumnBase(std::string name, int pos, DataType dataType) :
      name_(name), position_(pos), dataType_(dataType) {}
    const string& GetName() const { return name_; }
    int GetPosition() { return position_; }
    DataType GetDataType() { return dataType_; }

    virtual uint32_t ReadData(char* pBuf, uint32_t len) = 0;
    virtual uint32_t WriteData(char* pBuf, uint32_t len) = 0;
    virtual bool IsNullable() const = 0;
    virtual uint32_t GetMaxLength() const = 0;
  protected:
    string name_;
    int position_;
    DataType dataType_;
  };

  class ColumnInTable :public ColumnBase {
  public:
    ColumnInTable(const std::string& name, int pos, DataType dataType, const string& comments, bool bNullable,
      bool bIndex, int maxLen, int incStep, utils::Charsets charset, IDataValue* defaultVal);
    ~ColumnInTable();
  public:
    virtual uint32_t ReadData(char* pBuf, uint32_t len);
    virtual uint32_t WriteData(char* pBuf, uint32_t len);
    bool IsNullable() const { return bNullable_; }
    uint32_t GetMaxLength() const { return maxLength_; }
    uint32_t GetIncStep() const { return incStep_; }
    bool IsIndex() const { return bIndex_; }
    utils::Charsets GetCharset() { return charset_; }
    const IDataValue* getDefaultVal() { return defaultVal_; }
    const string& GetComments() { return comments_; }
  protected:
    bool bNullable_;
    uint32_t maxLength_;
    uint32_t incStep_;
    bool bIndex_;
    utils::Charsets charset_;
    IDataValue* defaultVal_;
    string comments_;
  };
}