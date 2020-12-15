#pragma once
#include <string>
#include <any>
#include "../dataType/DataType.h"
#include "../utils/CharsetConvert.h"
#include "../dataType/IDataValue.h"

namespace storage {
  class IDataValue;
  using namespace std;
  class ColumnBase {
  public:
    ColumnBase(){ }
    ColumnBase(std::string name, uint32_t pos, DataType dataType) :
      name_(name), position_(pos), dataType_(dataType) {}
    const string& GetName() const { return name_; }
    uint32_t GetPosition() { return position_; }
    DataType GetDataType() { return dataType_; }

    virtual uint32_t ReadData(char* pBuf) = 0;
    virtual uint32_t WriteData(char* pBuf) = 0;
    virtual bool IsNullable() const = 0;
    virtual uint32_t GetMaxLength() const = 0;
  protected:
    string name_;
    uint32_t position_;
    DataType dataType_;
  };

  class ColumnInTable :public ColumnBase {
  public:
    ColumnInTable() {};
    ColumnInTable(const std::string& name, uint32_t pos, DataType dataType, const string& comments, bool bNullable,
      bool bIndex, uint32_t maxLen, uint32_t incStep, utils::Charsets charset, IDataValue* defaultVal);
    ~ColumnInTable();
  public:
    virtual uint32_t ReadData(char* pBuf);
    virtual uint32_t WriteData(char* pBuf);
    bool IsNullable() const { return bNullable_; }
    uint32_t GetMaxLength() const { return maxLength_; }
    uint32_t GetIncStep() const { return incStep_; }
    bool IsIndex() const { return bIndex_; }
    utils::Charsets GetCharset() { return charset_; }
    const IDataValue* getDefaultVal() { return pDefaultVal_; }
    const string& GetComments() { return comments_; }
  protected:
    bool bNullable_;
    uint32_t maxLength_;
    uint32_t incStep_;
    bool bIndex_;
    utils::Charsets charset_;
    string comments_;
    IDataValue* pDefaultVal_;
  };
}