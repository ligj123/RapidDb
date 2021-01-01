#include "DataValueFixChar.h"
#include <stdexcept>
#include "../utils/ErrorMsg.h"

namespace storage {
  DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::NULL_VALUE, bKey), maxLength_(maxLength)
  { }

  DataValueFixChar::DataValueFixChar(char* val, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey), soleValue_(val),
      soleLength_((uint32_t)strlen(val) + 1), maxLength_(maxLength)
  {
    if (soleLength_ >= maxLength_)
    {
      delete[] val;
      soleValue_ = nullptr;
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    }
  }

  DataValueFixChar::DataValueFixChar(const char* val, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey),
      soleLength_((uint32_t)strlen(val) + 1), maxLength_(maxLength)
  {
    if (soleLength_ >= maxLength_)
    {
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    }
    soleValue_ = new char[soleLength_];
    memcpy(soleValue_, val, soleLength_);
  }

  DataValueFixChar::DataValueFixChar(Byte* byArray, uint32_t strLen, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::BYTES_VALUE, bKey), byArray_(byArray),
      soleLength_(strLen), maxLength_(maxLength)
  { }

  DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey, std::any val)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey), maxLength_(maxLength)
  {
    string str;
    if (val.type() == typeid(int64_t)) str = move(to_string(any_cast<int64_t>(val)));
    else if (val.type() == typeid(int64_t)) str = move(to_string(any_cast<int64_t>(val)));
    else if (val.type() == typeid(int32_t)) str = move(to_string(any_cast<int32_t>(val)));
    else if (val.type() == typeid(int16_t)) str = move(to_string(any_cast<int16_t>(val)));
    else if (val.type() == typeid(uint64_t)) str = move(to_string(any_cast<uint64_t>(val)));
    else if (val.type() == typeid(uint32_t)) str = move(to_string(any_cast<uint32_t>(val)));
    else if (val.type() == typeid(uint16_t)) str = move(to_string(any_cast<uint16_t>(val)));
    else if (val.type() == typeid(int8_t)) str = move(to_string(any_cast<int8_t>(val)));
    else if (val.type() == typeid(uint8_t)) str = move(to_string(any_cast<uint8_t>(val)));
    else if (val.type() == typeid(std::string)) str = move(any_cast<std::string>(val));
    else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueFixChar" });

    soleLength_ = (uint32_t)str.size() + 1;
    if (soleLength_ >= maxLength_)
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    
    soleValue_ = new char[soleLength_];
    memcpy(soleValue_, str.c_str(), soleLength_);
  }

  DataValueFixChar::DataValueFixChar(const DataValueFixChar& src) : IDataValue(src)
  {
    maxLength_ = src.maxLength_;
    soleLength_ = src.soleLength_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = new char[soleLength_];
      memcpy(soleValue_, src.soleValue_, soleLength_);
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
    if (valType_ == ValueType::SOLE_VALUE)
    {
      delete[] soleValue_;
      valType_ = ValueType::NULL_VALUE;
    }
  }

  std::any DataValueFixChar::GetValue() const
  {
    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      return string(soleValue_);
    case ValueType::BYTES_VALUE:
        return string((char*)byArray_);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t DataValueFixChar::WriteData(Byte* buf)
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
        std::memcpy(buf, soleValue_, strlen(soleValue_));
        std::memset(buf + strlen(soleValue_), '\0', maxLength_ - strlen(soleValue_));
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
       
        std::memcpy(buf, soleValue_, soleLength_);
        std::memset(buf + soleLength_, '\0', maxLength_ - soleLength_);
        return maxLength_ + 1;
      }
    }
  }
  uint32_t DataValueFixChar::ReadData(Byte* buf, uint32_t len)
  {
    if (bKey_)
    {
      valType_ = ValueType::BYTES_VALUE;
      byArray_ = buf;
      soleLength_ = (uint32_t)strlen((char*)buf) + 1;
      return maxLength_;
    }
    else
    {
      valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
      buf++;

      if (valType_ == ValueType::NULL_VALUE)
        return 1;

      byArray_ = buf;
      soleLength_ = (uint32_t)strlen((char*)buf) + 1;
      return maxLength_ + 1;
    }
  }

  uint32_t DataValueFixChar::GetDataLength() const
  {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return soleLength_;
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
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[1]{};
    soleLength_ = 1;
  }

  void DataValueFixChar::SetMaxValue()
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[] { "\\uff\\uff\\uff"};
    soleLength_ = 4;
  }

  void DataValueFixChar::SetDefaultValue()
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[1]{0};
    soleLength_ = 1;
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
      return string((char*)byArray_);
    }

    return "";
  }

  DataValueFixChar& DataValueFixChar::operator=(char* val)
  {
    uint32_t len = strlen(val) + 1;
    if (len >= maxLength_)
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = val;
    soleLength_ = len;
    return *this;
  }

  DataValueFixChar& DataValueFixChar::operator=(const char* val)
  {
    uint32_t len = strlen(val) + 1;
    if (len >= maxLength_ - 1)
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    soleLength_ = len;
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[len];
    memcpy(soleValue_, val, len);
    return *this;
  }

  DataValueFixChar& DataValueFixChar::operator=(const DataValueFixChar& src)
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    dataType_ = src.dataType_;
    valType_ = src.valType_;
    bKey_ = src.bKey_;
    maxLength_ = src.maxLength_;
    soleLength_ = src.soleLength_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = new char[soleLength_];
      memcpy(soleValue_, src.soleValue_, soleLength_);
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

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : (char*)dv.byArray_);
    int rt = strncmp(v1, v2, std::min(GetDataLength(), dv.GetDataLength()));
    if (rt == 0)
    {
      return GetDataLength() > dv.GetDataLength();
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

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : (char*)dv.byArray_);
    int rt = strncmp(v1, v2, std::min(GetDataLength(), dv.GetDataLength()));
    if (rt == 0)
    {
      return GetDataLength() >= dv.GetDataLength();
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
    
    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : (char*)dv.byArray_);
    return strncmp(v1, v2, std::min(GetDataLength(), dv.GetDataLength())) == 0;
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
      os << (char*)dv.byArray_;
      break;
    }

    return os;
  }
}
