#include "DataValueFixChar.h"
#include <stdexcept>
#include "../utils/ErrorMsg.h"

namespace storage {
  DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey, utils::Charsets charset)
    : IDataValue(DataType::FIXCHAR, ValueType::NULL_VALUE, bKey), maxLength_(maxLength), charset_(charset)
  { }

  DataValueFixChar::DataValueFixChar(string val, uint32_t maxLength, bool bKey, utils::Charsets charset)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey), soleValue_(val),
      maxLength_(maxLength), charset_(charset)
  { }

  DataValueFixChar::DataValueFixChar(uint32_t maxLength, char* byArray, bool bKey, utils::Charsets charset)
    : IDataValue(DataType::FIXCHAR, ValueType::BYTES_VALUE, bKey), byArray_(byArray),
      maxLength_(maxLength), charset_(charset)
  { }

  DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey, utils::Charsets charset, std::any val)
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

  DataValueFixChar::DataValueFixChar(const DataValueFixChar& src) : IDataValue(src)
  {
    maxLength_ = src.maxLength_;
    charset_ = src.charset_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = src.soleValue_;
      break;
    case ValueType::BYTES_VALUE:
      byArray_ = src.byArray_;
      break;
    case ValueType::NULL_VALUE:
    default:
      break;
    }
  }

  DataValueFixChar::~DataValueFixChar()
  {
  }

  std::any DataValueFixChar::GetValue() const
  {
    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      if (byArray_[maxLength_ - 1] == 0)
        return string(byArray_);
      else
        return string(byArray_, maxLength_);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t DataValueFixChar::WriteData(char* buf)
  {
    if (bKey_)
    {
      if (valType_ == ValueType::BYTES_VALUE)
      {
        std::memcpy(buf, byArray_, maxLength_);
        return maxLength_;
      }
      else if (valType_ == ValueType::SOLE_VALUE)
      {
        std::memcpy(buf, soleValue_.c_str(), soleValue_.size());
        std::memset(buf + soleValue_.size(), '\0', maxLength_ - soleValue_.size());
        return maxLength_;
      }
      else
      {
        std::memset(buf, '\0', maxLength_);
        return maxLength_;
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
        std::memcpy(buf, byArray_, maxLength_);
        return maxLength_ + 1;
      }
      else
      {
        *buf = 1;
        buf++;
       
        std::memcpy(buf, soleValue_.c_str(), soleValue_.size());
        std::memset(buf + soleValue_.size(), '\0', maxLength_ - soleValue_.size());
        return maxLength_ + 1;
      }
    }
  }
  uint32_t DataValueFixChar::ReadData(char* buf, uint32_t len)
  {
    if (bKey_)
    {
      valType_ = ValueType::BYTES_VALUE;
      byArray_ = buf;
      return maxLength_;
    }
    else
    {
      valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
      buf++;

      if (valType_ == ValueType::NULL_VALUE)
        return 1;

      byArray_ = buf;
      return maxLength_ + 1;
    }
  }

  uint32_t DataValueFixChar::GetLength() const
  {
    if (bKey_)
    {
      return maxLength_;
    }
    else
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return maxLength_;
      case ValueType::BYTES_VALUE:
        return maxLength_;
      case ValueType::NULL_VALUE:
      default:
        return 0;
      }
    }
  }

  uint32_t DataValueFixChar::GetMaxLength() const
  {
    return maxLength_;
  }

  uint32_t DataValueFixChar::GetPersistenceLength() const
  {
    if (bKey_)
    {
      return maxLength_;
    }
    else
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return maxLength_ + 1;
      case ValueType::BYTES_VALUE:
        return maxLength_ + 1;
      case ValueType::NULL_VALUE:
      default:
        return 1;
      }
    }
  }

  void DataValueFixChar::SetMinValue()
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = "";
  }

  void DataValueFixChar::SetMaxValue()
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = "\\uff\\uff\\uff";
  }

  void DataValueFixChar::SetDefaultValue()
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = "";
  }

  DataValueFixChar::operator string() const
  {
    switch (valType_)
    {
    case ValueType::NULL_VALUE:
      return "";
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      if (byArray_[maxLength_ - 1] == 0)
        return string(byArray_);
      else
        return string(byArray_, maxLength_);
    }

    return "";
  }

  DataValueFixChar& DataValueFixChar::operator=(const string& val)
  {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = val;
    return *this;
  }

  DataValueFixChar& DataValueFixChar::operator=(const DataValueFixChar& src)
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
      byArray_ = src.byArray_;
      break;
    case ValueType::NULL_VALUE:
    default:
      break;
    }

    return *this;
  }

  bool DataValueFixChar::operator > (const DataValueFixChar& dv) const
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

  bool DataValueFixChar::operator < (const DataValueFixChar& dv) const
  {
    return !(*this >= dv);
  }

  bool DataValueFixChar::operator >= (const DataValueFixChar& dv) const
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

  bool DataValueFixChar::operator <= (const DataValueFixChar& dv) const
  {
    return !(*this > dv);
  }

  bool DataValueFixChar::operator == (const DataValueFixChar& dv) const
  {
    if (valType_ == ValueType::NULL_VALUE) { return dv.valType_ == ValueType::NULL_VALUE; }
    if (dv.valType_ == ValueType::NULL_VALUE) { return false; }
    
    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_.c_str() : byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_.c_str() : dv.byArray_);
    return strncmp(v1, v2, std::min(GetLength(), dv.GetLength())) == 0;
  }

  bool DataValueFixChar::operator != (const DataValueFixChar& dv) const
  {
    return !(*this == dv);
  }

  std::ostream& operator<< (std::ostream& os, const DataValueFixChar& dv)
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
      if (dv.byArray_[dv.maxLength_ - 1] == 0)
        os << string(dv.byArray_);
      else
        os << string(dv.byArray_, dv.maxLength_);
      break;
    }

    return os;
  }
}
