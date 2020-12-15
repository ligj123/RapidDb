#include "DataValueVarChar.h"
#include <stdexcept>
#include "../utils/ErrorMsg.h"

namespace storage {
  DataValueVarChar::DataValueVarChar(uint32_t maxLength, bool bKey, utils::Charsets charset)
    : IDataValue(DataType::VARCHAR, ValueType::NULL_VALUE, bKey), maxLength_(maxLength), charset_(charset)
  { }
  DataValueVarChar::DataValueVarChar(string val, uint32_t maxLength, bool bKey, utils::Charsets charset)
    : IDataValue(DataType::VARCHAR, ValueType::SOLE_VALUE, bKey), soleValue_(val),
      maxLength_(maxLength), charset_(charset)
  { }

  DataValueVarChar::DataValueVarChar(char* byArray, uint32_t len, uint32_t maxLength, bool bKey, utils::Charsets charset)
    : IDataValue(DataType::VARCHAR, ValueType::BYTES_VALUE, bKey), byArray_(byArray),
      maxLength_(maxLength), len_(len), charset_(charset)
  { }

  DataValueVarChar::DataValueVarChar(uint32_t maxLength, bool bKey, utils::Charsets charset, std::any val)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey), maxLength_(maxLength), charset_(charset)
  {
    if (val.type() == typeid(int64_t)) soleValue_ = std::to_string(std::any_cast<int64_t>(val));
    else if (val.type() == typeid(int64_t)) soleValue_ = std::to_string(std::any_cast<int64_t>(val));
    else if (val.type() == typeid(int32_t)) soleValue_ = std::to_string(std::any_cast<int32_t>(val));
    else if (val.type() == typeid(int16_t)) soleValue_ = std::to_string(std::any_cast<int16_t>(val));
    else if (val.type() == typeid(uint64_t)) soleValue_ = std::to_string(std::any_cast<uint64_t>(val));
    else if (val.type() == typeid(uint32_t)) soleValue_ = std::to_string(std::any_cast<uint32_t>(val));
    else if (val.type() == typeid(uint16_t)) soleValue_ = std::to_string(std::any_cast<uint16_t>(val));
    else if (val.type() == typeid(int8_t)) soleValue_ = std::to_string(std::any_cast<int8_t>(val));
    else if (val.type() == typeid(uint8_t)) soleValue_ = std::to_string(std::any_cast<uint8_t>(val));
    else if (val.type() == typeid(std::string)) soleValue_ = std::any_cast<std::string>(val);
    else throw utils::ErrorMsg(2001, { val.type().name(), "string" });
  }

  DataValueVarChar::DataValueVarChar(const DataValueVarChar& src) : IDataValue(src)
  {
    maxLength_ = src.maxLength_;
    charset_ = src.charset_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = src.soleValue_;
      break;
    case ValueType::BYTES_VALUE:
      len_ = src.len_;
      byArray_ = src.byArray_;
      break;
    case ValueType::NULL_VALUE:
    default:
      break;
    }
  }

  DataValueVarChar::~DataValueVarChar()
  {
  }

  std::any DataValueVarChar::GetValue() const
  {
    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      return string(byArray_);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t DataValueVarChar::WriteData(char* buf)
  {
    if (bKey_)
    {
      if (valType_ == ValueType::BYTES_VALUE)
      {
        std::memcpy(buf, byArray_, len_);
        return len_;
      }
      else if (valType_ == ValueType::SOLE_VALUE)
      {
        std::memcpy(buf, soleValue_.c_str(), soleValue_.size());
        buf[soleValue_.size()] = '\0';
        return (uint32_t)soleValue_.size() + 1;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      if (valType_ == ValueType::NULL_VALUE)
      {
        *buf = 0;
        return 1;
      }
      else if (valType_ == ValueType::BYTES_VALUE)
      {
        *buf = 1;
        buf++;
        *((uint32_t*)buf) = len_;
        buf += sizeof(uint32_t);
        std::memcpy(buf, byArray_, len_);
        return len_ + sizeof(uint32_t) + 1;
      }
      else
      {
        *buf = 1;
        buf++;
        *((uint32_t*)buf) = (uint32_t)soleValue_.size() + 1;
        buf += sizeof(uint32_t);
        std::memcpy(buf, soleValue_.c_str(), soleValue_.size());
        buf[soleValue_.size()] = '\0';
        return (uint32_t)soleValue_.size() + sizeof(uint32_t) + 2;
      }
    }
  }
  uint32_t DataValueVarChar::ReadData(char* buf, uint32_t len)
  {
    if (bKey_)
    {
      if (len == 0)
      {
        valType_ = ValueType::NULL_VALUE;
        return 0;
      }

      valType_ = ValueType::BYTES_VALUE;
      byArray_ = buf;
      len_ = len;
      return len;
    }
    else
    {
      valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
      buf++;

      if (valType_ == ValueType::NULL_VALUE)
        return 1;

      len_ = *((uint32_t*)buf);
      buf += sizeof(uint32_t);
      byArray_ = buf;
      return len_ + sizeof(uint32_t) + 1;
    }
  }

  uint32_t DataValueVarChar::GetLength() const
  {
    if (bKey_)
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return (uint32_t)soleValue_.size() + 1;
      case ValueType::BYTES_VALUE:
        return len_;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
    else
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return (uint32_t)soleValue_.size() + 1;
      case ValueType::BYTES_VALUE:
        return len_;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
  }

  uint32_t DataValueVarChar::GetMaxLength() const
  {
    return maxLength_;
  }

  uint32_t DataValueVarChar::GetPersistenceLength() const
  {
    if (bKey_)
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return (uint32_t)soleValue_.size() + 1;
      case ValueType::BYTES_VALUE:
        return len_;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
    else
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return (uint32_t)soleValue_.size() + 2 + sizeof(uint32_t);
      case ValueType::BYTES_VALUE:
        return len_ + 1 + sizeof(uint32_t);
      case ValueType::NULL_VALUE:
      default:
        return 1;
      }
    }
  }
  void DataValueVarChar::SetMinValue()
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = "";
  }

  void DataValueVarChar::SetMaxValue()
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = "\\uff\\uff\\uff";
  }
  void DataValueVarChar::SetDefaultValue()
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = "";
  }

  DataValueVarChar::operator string() const
  {
    switch (valType_)
    {
    case ValueType::NULL_VALUE:
      return "";
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      return string(byArray_);
    }

    return "";
  }

  DataValueVarChar& DataValueVarChar::operator=(const string& val)
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = val;
    return *this;
  }

  DataValueVarChar& DataValueVarChar::operator=(const DataValueVarChar& src)
  {
    dataType_ = src.dataType_;
    valType_ = src.valType_;
    bKey_ = src.bKey_;
    maxLength_ = src.maxLength_;
    charset_ = src.charset_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = src.soleValue_;
      break;
    case ValueType::BYTES_VALUE:
      len_ = src.len_;
      byArray_ = src.byArray_;
      break;
    case ValueType::NULL_VALUE:
    default:
      break;
    }

    return *this;
  }

  bool DataValueVarChar::operator > (const DataValueVarChar& dv) const
  {
    if (valType_ == ValueType::NULL_VALUE) { return false; }
    if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_.c_str() : byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_.c_str() : dv.byArray_);
    int rt = strncmp(v1, v2, std::min(GetLength(), dv.GetLength()));
    if (rt == 0)
    {
      return GetLength() > dv.GetLength();
    }
    else
    {
      return rt > 0;
    }
  }

  bool DataValueVarChar::operator < (const DataValueVarChar& dv) const
  {
    return !(*this >= dv);
  }

  bool DataValueVarChar::operator >= (const DataValueVarChar& dv) const
  {
    if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
    if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_.c_str() : byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_.c_str() : dv.byArray_);
    int rt = strncmp(v1, v2, std::min(GetLength(), dv.GetLength()));
    if (rt == 0)
    {
      return GetLength() >= dv.GetLength();
    }
    else
    {
      return rt > 0;
    }
  }

  bool DataValueVarChar::operator <= (const DataValueVarChar& dv) const
  {
    return !(*this > dv);
  }

  bool DataValueVarChar::operator == (const DataValueVarChar& dv) const
  {
    if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
    if (dv.valType_ == ValueType::NULL_VALUE) { return false; }
    
    if (GetLength() != dv.GetLength()) return false;

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_.c_str() : byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_.c_str() : dv.byArray_);
    return strncmp(v1, v2, GetLength()) == 0;
  }

  bool DataValueVarChar::operator != (const DataValueVarChar& dv) const
  {
    return !(*this == dv);
  }

  std::ostream& operator<< (std::ostream& os, const DataValueVarChar& dv)
  {
    switch (dv.valType_)
    {
    case ValueType::NULL_VALUE:
      os << "nullptr";
      break;
    case ValueType::SOLE_VALUE:
      os << dv.soleValue_;
      break;
    case ValueType::BYTES_VALUE:
      os << string(dv.byArray_, dv.len_);
      break;
    }

    return os;
  }
}
