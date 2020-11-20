#pragma once
#include <string>
#include <any>
#include "../dataType/DataType.h"
#include "../utils/CharsetConvert.h"

namespace storage {
  class Column {
  public:
  protected:
    std::string name_;
    int position_;
    bool bNullable_;
    DataType dataType_;
    int maxLength_;
    std::string comments_;
    int incStep;
    bool bIndex;
    utils::Charsets charset_;
    std::any defaultVal_;
  };
}