#pragma once
#include "Column.h"
#include "../dataType/DataValueLong.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/DataValueFixChar.h"

namespace storage {
  ColumnInTable::ColumnInTable(const std::string& name, uint32_t pos, DataType dataType, const string& comments, bool bNullable,
    bool bIndex, uint32_t maxLen, uint32_t incStep, utils::Charsets charset, IDataValue* defaultVal) :
    ColumnBase(name, pos, dataType), bNullable_(bNullable), bIndex_(bIndex), comments_(comments),
    maxLength_(maxLen), incStep_(incStep), charset_(charset), pDefaultVal_(defaultVal) {}
  ColumnInTable::~ColumnInTable() {
    delete pDefaultVal_;
  }

  uint32_t ColumnInTable::ReadData(char* pBuf)
  {
    char* p = pBuf;
    *((uint32_t*)p) = (uint32_t)name_.size();
    p += sizeof(uint32_t);
    std::memcpy(p, name_.c_str(), name_.size());
    p += name_.size();

    *((uint32_t*)p) = position_;
    p += sizeof(uint32_t);

    *((uint32_t*)p) = (uint32_t)dataType_;
    p += sizeof(uint32_t);

    *p = (bNullable_ ? 0 : 1);
    p++;

    *((uint32_t*)p) = maxLength_;
    p += sizeof(uint32_t);

    *((uint32_t*)p) = incStep_;
    p += sizeof(uint32_t);

    *p = (bIndex_ ? 1 : 0);
    p++;

    *((uint32_t*)p) = (uint32_t)charset_;
    p += sizeof(uint32_t);

    *((uint32_t*)p) = (uint32_t)comments_.size();
    p += sizeof(uint32_t);
    std::memcpy(p, comments_.c_str(), comments_.size());
    p += comments_.size();

    *p = (pDefaultVal_ == nullptr ? 0 : 1);
    p++;
    if (pDefaultVal_ != nullptr)
    {
      p += pDefaultVal_->WriteData(p);
    }

    return (int32_t)(p - pBuf);
  }

  uint32_t ColumnInTable::WriteData(char* pBuf)
  {
    char* p = pBuf;

    uint32_t len = *((uint32_t*)p);
    p += sizeof(uint32_t);
    name_ = string(p, len);
    p += len;

    position_ = *((uint32_t*)p);
    p += sizeof(uint32_t);

    dataType_ = (DataType)*((uint32_t*)p);
    p += sizeof(uint32_t);

    bNullable_ = (*p == 0);
    p++;

    maxLength_ = *((uint32_t*)p);
    p += sizeof(uint32_t);

    incStep_ = *((uint32_t*)p);
    p += sizeof(uint32_t);

    bIndex_ = (*p != 0);
    p++;

    charset_ = (utils::Charsets)*((uint32_t*)p);
    p += sizeof(uint32_t);

    len = *((uint32_t*)p);
    p += sizeof(uint32_t);
    comments_ = string(p, len);
    p += len;

    bool bDefault = (*p != 0);
    p++;
    if (bDefault)
    {
      switch (dataType_)
      {
      case DataType::LONG:
        pDefaultVal_ = new DataValueLong(*((int64_t*)p));
        p += sizeof(uint64_t);
        break;
      case DataType::VARCHAR:
        len = (*(int32_t*)p);
        p += sizeof(uint32_t);
        pDefaultVal_ = new DataValueVarChar(p);
        p += len;
        break;
      case DataType::FIXCHAR:
        if (p[maxLength_ - 1] == '\0')
          pDefaultVal_ = new DataValueFixChar(p);
        else 
          pDefaultVal_ = new DataValueFixChar(p, maxLength_);
        p += maxLength_;
      }
    }

    return (uint32_t)(p - pBuf);
  }
}