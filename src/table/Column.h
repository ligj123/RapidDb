#pragma once
#include <string>
#include <any>
#include "../dataType/DataType.h"
#include "../utils/CharsetConvert.h"
#include "../dataType/IDataValue.h"

namespace storage {
  using namespace std;
  class ColumnBase {
  public:
    ColumnBase(std::string name, int pos, DataType dataType) :
      name_(name), position_(pos), dataType_(dataType) {}
    const string& GetName() const { return name_; }
    int GetPosition() { return position_; }
    DataType GetDataType() { return dataType_; }

    virtual int readData(char* pBuf, int len) = 0;
    virtual int writeData(char* pBuf, int len) = 0;
  protected:
    string name_;
    int position_;
    DataType dataType_;
  };

  class ColumnInTable :public ColumnBase {
  public:
    ColumnInTable(const std::string& name, int pos, DataType dataType, const string& comments, bool bNullable,
      bool bIndex, int maxLen, int incStep, utils::Charsets charset, IDataValue* defaultVal):
      ColumnBase(name, pos, dataType), bNullable_(bNullable), bIndex_(bIndex), comments_(comments),
      maxLength_(maxLen), incStep_(incStep), charset_(charset), defaultVal_(defaultVal){}
    const string& GetComments() { return comments_; }
  protected:
    bool bNullable_;
    int maxLength_;
    int incStep_;
    bool bIndex_;
    utils::Charsets charset_;
    IDataValue* defaultVal_;
    string comments_;
  };
}