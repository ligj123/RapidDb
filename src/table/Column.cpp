#pragma once
#include "Column.h"
#include "../dataType/IDataValue.h"

namespace storage {
  ColumnInTable::ColumnInTable(const std::string& name, int pos, DataType dataType, const string& comments, bool bNullable,
    bool bIndex, int maxLen, int incStep, utils::Charsets charset, IDataValue* defaultVal) :
    ColumnBase(name, pos, dataType), bNullable_(bNullable), bIndex_(bIndex), comments_(comments),
    maxLength_(maxLen), incStep_(incStep), charset_(charset), defaultVal_(defaultVal) {}
  ColumnInTable::~ColumnInTable() {
    delete defaultVal_;
  }
  uint32_t ColumnInTable::ReadData(char* pBuf, uint32_t len)
  {
    return 0;
  }
  uint32_t ColumnInTable::WriteData(char* pBuf, uint32_t len)
  {
    return 0;
  }
}