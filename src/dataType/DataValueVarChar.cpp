#include "DataValueVarChar.h"
#include <stdexcept>

namespace storage {
  DataValueVarChar::DataValueVarChar(int maxLen, Charsets set)
    : TDataValue<string>(DataType::VARCHAR, maxLen), charset_(set)
  { }
  DataValueVarChar::DataValueVarChar(string str, int maxLen, Charsets set)
    : TDataValue<string>(DataType::VARCHAR, str, maxLen), charset_(set)
  { }

  DataValueVarChar::DataValueVarChar(char* byArray, int len, int maxLen, Charsets set)
    : TDataValue<string>(DataType::VARCHAR, byArray, len, maxLen), charset_(set)
  { }

  std::any DataValueVarChar::GetValue() const
  {
    switch (valType_)
    {
    case ValueType::SOLE_VALUE:
      return soleValue_;
    case ValueType::BYTES_VALUE:
      return string(byArray_, len_);
    case ValueType::NULL_VALUE:
    default:
      return std::any();
    }
  }

  int DataValueVarChar::WriteData(char* buf, bool bkey)
  {
    return 0;
  }
  int DataValueVarChar::ReadData(char* buf, bool bkey, int len)
  {
    return 0;
  }
  int DataValueVarChar::GetLength(bool bKey) const
  {
    if (bKey)
    {
      switch (valType_)
      {
      case ValueType::SOLE_VALUE:
        return soleValue_.size();
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
        return soleValue_.size() + 1 + sizeof(int32_t);
      case ValueType::BYTES_VALUE:
        return len_ + 1 + sizeof(int32_t);
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
        return string(byArray_, len_);
      }

      return "";
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
