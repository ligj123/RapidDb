#pragma once
#include <string>
#include <any>
#include "../dataType/DataType.h"
#include "../utils/CharsetConvert.h"
#include "../dataType/IDataValue.h"
#include "../header.h"

namespace storage {
class IDataValue;
using namespace std;
class BaseColumn {
public:
  BaseColumn() { }
  BaseColumn(std::string name, uint32_t pos, DataType dataType) :
    name_(name), position_(pos), dataType_(dataType) {}
  const string& GetName() const { return name_; }
  uint32_t GetPosition() { return position_; }
  DataType GetDataType() { return dataType_; }

  virtual uint32_t ReadData(Byte* pBuf) = 0;
  virtual uint32_t WriteData(Byte* pBuf) = 0;
protected:
  string name_;
  uint32_t position_;
  DataType dataType_;
};

class PersistColumn :public BaseColumn {
public:
  PersistColumn() {};
  PersistColumn(const std::string& name, uint32_t pos, DataType dataType, const string& comments, bool bNullable,
    uint32_t maxLen, uint32_t incStep, utils::Charsets charset, IDataValue* defaultVal);
  ~PersistColumn();
public:
  uint32_t ReadData(Byte* pBuf) override;
  uint32_t WriteData(Byte* pBuf) override;
  bool IsNullable() const { return bNullable_; }
  uint32_t GetMaxLength() const { return maxLength_; }
  uint32_t GetIncStep() const { return incStep_; }
  utils::Charsets GetCharset() { return charset_; }
  const IDataValue* getDefaultVal() { return pDefaultVal_; }
  const string& GetComments() { return comments_; }
protected:
  bool bNullable_;
  uint32_t maxLength_;
  uint32_t incStep_;
  utils::Charsets charset_;
  string comments_;
  IDataValue* pDefaultVal_;
};
}
