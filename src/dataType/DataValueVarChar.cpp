#include "DataValueVarChar.h"
#include <stdexcept>
#include "../utils/ErrorMsg.h"

namespace storage {
  DataValueVarChar::DataValueVarChar(uint32_t maxLength, bool bKey)
    : IDataValue(DataType::VARCHAR, ValueType::NULL_VALUE, bKey), maxLength_(maxLength)
  { }
  DataValueVarChar::DataValueVarChar(char* val, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::VARCHAR, ValueType::SOLE_VALUE, bKey), soleValue_(val),
      maxLength_(maxLength), soleLength_(strlen(val) + 1)
  {
    if (soleLength_ >= maxLength_ - 1)
    {
      delete[] val;
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    }
  }

  DataValueVarChar::DataValueVarChar(Byte* byArray, uint32_t strlen, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::VARCHAR, ValueType::BYTES_VALUE, bKey), byArray_(byArray),
      maxLength_(maxLength), soleLength_(strlen)
  { }

  DataValueVarChar::DataValueVarChar(uint32_t maxLength, bool bKey, std::any val)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey), maxLength_(maxLength)
  {
    string str;
    if (val.type() == typeid(int64_t)) str = move(to_string(std::any_cast<int64_t>(val)));
    else if (val.type() == typeid(int64_t)) str = move(to_string(std::any_cast<int64_t>(val)));
    else if (val.type() == typeid(int32_t)) str = move(to_string(std::any_cast<int32_t>(val)));
    else if (val.type() == typeid(int16_t)) str = move(to_string(std::any_cast<int16_t>(val)));
    else if (val.type() == typeid(uint64_t)) str = move(to_string(std::any_cast<uint64_t>(val)));
    else if (val.type() == typeid(uint32_t)) str = move(to_string(std::any_cast<uint32_t>(val)));
    else if (val.type() == typeid(uint16_t)) str = move(to_string(std::any_cast<uint16_t>(val)));
    else if (val.type() == typeid(int8_t)) str = move(to_string(std::any_cast<int8_t>(val)));
    else if (val.type() == typeid(uint8_t)) str = move(to_string(std::any_cast<uint8_t>(val)));
    else if (val.type() == typeid(std::string)) str = std::any_cast<std::string>(val);
    else throw utils::ErrorMsg(2001, { val.type().name(), "DataValueVarChar" });

    soleLength_ = str.size() + 1;
    if (soleLength_ >= maxLength_)
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });

    soleValue_ = new char[soleLength_];
    strcpy(soleValue_, str.c_str());
  }

  DataValueVarChar::DataValueVarChar(const DataValueVarChar& src) : IDataValue(src)
  {
    maxLength_ = src.maxLength_;
    soleLength_ = src.soleLength_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = new char[strlen(src.soleValue_) + 1];
      strcpy(soleValue_, src.soleValue_);
      break;
    case ValueType::BYTES_VALUE:
      byArray_ = src.byArray_;
      break;
    case ValueType::NULL_VALUE:
    default:
      break;
    }
  }

  DataValueVarChar::~DataValueVarChar()
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;
  }

  std::any DataValueVarChar::GetValue() const
  {
    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      return (char*)byArray_;
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  uint32_t DataValueVarChar::WriteData(Byte* buf)
  {
    if (bKey_)
    {
      if (valType_ == ValueType::BYTES_VALUE)
      {
        std::memcpy(buf, byArray_, soleLength_);
        return soleLength_;
      }
      else if (valType_ == ValueType::SOLE_VALUE)
      {
        std::memcpy(buf, soleValue_, soleLength_);
        buf[soleLength_] = '\0';
        return (uint32_t)soleLength_;
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
        *((uint32_t*)buf) = soleLength_;
        buf += sizeof(uint32_t);
        std::memcpy(buf, byArray_, soleLength_);
        return soleLength_ + sizeof(uint32_t) + 1;
      }
      else
      {
        *buf = 1;
        buf++;
        *((uint32_t*)buf) = soleLength_;
        buf += sizeof(uint32_t);
        std::memcpy(buf, soleValue_, soleLength_);
        return (uint32_t)soleLength_ + sizeof(uint32_t) + 1;
      }
    }
  }
  uint32_t DataValueVarChar::ReadData(Byte* buf, uint32_t len)
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
      soleLength_ = len;
      return len;
    }
    else
    {
      valType_ = (*buf ? ValueType::BYTES_VALUE : ValueType::NULL_VALUE);
      buf++;

      if (valType_ == ValueType::NULL_VALUE)
        return 1;

      soleLength_ = *((uint32_t*)buf);
      buf += sizeof(uint32_t);
      byArray_ = buf;
      return soleLength_ + sizeof(uint32_t) + 1;
    }
  }

  uint32_t DataValueVarChar::GetLength() const
  {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return soleLength_;
  }

  uint32_t DataValueVarChar::GetMaxLength() const
  {
    return maxLength_;
  }

  uint32_t DataValueVarChar::GetPersistenceLength() const
  {
    if (bKey_)
    {
      if (valType_ == ValueType::NULL_VALUE)
        return 0;
      else
        return soleLength_;
    }
    else
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return soleLength_ + 1 + sizeof(uint32_t);
      case ValueType::BYTES_VALUE:
        return soleLength_ + 1 + sizeof(uint32_t);
      case ValueType::NULL_VALUE:
      default:
        return 1;
      }
    }
  }
  void DataValueVarChar::SetMinValue()
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[1]{};
    soleLength_ = 1;
  }

  void DataValueVarChar::SetMaxValue()
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[] { "\\uff\\uff\\uff"};
    soleLength_ = 4;
  }

  void DataValueVarChar::SetDefaultValue()
  {
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = new char[1]{ 0 };
    soleLength_ = 1;
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
      return string((char*)byArray_);
    }

    return "";
  }

  DataValueVarChar::operator char* () const
  {
    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      return (char*)byArray_;
    case ValueType::NULL_VALUE:
    default:
      return nullptr;
    }
  }

  DataValueVarChar& DataValueVarChar::operator=(char* val)
  {
    if (strlen(val) >= maxLength_ - 1)
      throw utils::ErrorMsg(2002, { to_string(maxLength_), to_string(soleLength_) });
    if (valType_ == ValueType::SOLE_VALUE) delete[] soleValue_;

    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = val;
    soleLength_ = strlen(val) + 1;
    return *this;
  }

  DataValueVarChar& DataValueVarChar::operator=(const DataValueVarChar& src)
  {
    dataType_ = src.dataType_;
    valType_ = src.valType_;
    bKey_ = src.bKey_;
    maxLength_ = src.maxLength_;
    soleLength_ = src.soleLength_;

    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      soleValue_ = new char[soleLength_];
      strcpy(soleValue_, src.soleValue_);
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

  bool DataValueVarChar::operator > (const DataValueVarChar& dv) const
  {
    if (valType_ == ValueType::NULL_VALUE) { return false; }
    if (dv.valType_ == ValueType::NULL_VALUE) { return true; }

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : (char*)dv.byArray_);
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

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : (char*)dv.byArray_);
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

    const char* v1 = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
    const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_ : (char*)dv.byArray_);
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
      os << (char*)dv.byArray_;
      break;
    }

    return os;
  }
}
