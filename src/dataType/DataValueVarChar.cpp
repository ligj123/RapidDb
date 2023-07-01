#include "DataValueVarChar.h"
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace storage {

bool DataValueVarChar::SetValue(const char *val, uint32_t len) {
  if (len >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len + 1)}));
    return false;
  }

  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = len + 1;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val, len);
  bysValue_[len] = 0;
  return true;
}

bool DataValueVarChar::PutValue(std::any val) {
  const char *buf;
  size_t len = 0;
  if (val.type() == typeid(string)) {
    buf = any_cast<string>(&val)->c_str();
    len = any_cast<string>(val).size();
  } else if (val.type() == typeid(MString)) {
    buf = any_cast<MString>(&val)->c_str();
    len = any_cast<MString>(val).size();
  } else if (val.type() == typeid(const char *))
    buf = any_cast<const char *>(val);
  else if (val.type() == typeid(char *))
    buf = any_cast<char *>(val);
  else if (val.type() == typeid(int64_t))
    buf = toChars(any_cast<int64_t>(val));
  else if (val.type() == typeid(int32_t))
    buf = toChars(any_cast<int32_t>(val));
  else if (val.type() == typeid(int16_t))
    buf = toChars(any_cast<int16_t>(val));
  else if (val.type() == typeid(uint64_t))
    buf = toChars(any_cast<uint64_t>(val));
  else if (val.type() == typeid(uint32_t))
    buf = toChars(any_cast<uint32_t>(val));
  else if (val.type() == typeid(uint16_t))
    buf = toChars(any_cast<uint16_t>(val));
  else if (val.type() == typeid(int8_t))
    buf = toChars(any_cast<int8_t>(val));
  else if (val.type() == typeid(uint8_t))
    buf = toChars(any_cast<uint8_t>(val));
  else if (val.type() == typeid(double))
    buf = toChars(any_cast<double>(val));
  else if (val.type() == typeid(float))
    buf = toChars(any_cast<float>(val));
  else {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_UNSUPPORT_CONVERT, {val.type().name(), "DataValueVarChar"}));
    return false;
  }

  if (len == 0)
    len = strlen(buf);

  if (len >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return false;
  }

  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = (uint32_t)len + 1;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, buf, soleLength_);
  return true;
}

bool DataValueVarChar::Copy(const IDataValue &dv, bool bMove) {
  if (dv.IsNull()) {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, soleLength_);
    }

    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
    return true;
  }

  if (!dv.IsStringType()) {
    StrBuff sb(0);
    dv.ToString(sb);
    if (maxLength_ < sb.GetStrLen() + 1) {
      _threadErrorMsg.reset(
          new ErrorMsg(DT_INPUT_OVER_LENGTH,
                       {to_string(maxLength_), to_string(sb.GetStrLen() + 1)}));
      return false;
    }

    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, soleLength_);
    }

    bysValue_ = CachePool::Apply(maxLength_);

    int len = sb.GetStrLen();
    soleLength_ = len + 1;
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, sb.GetBuff(), len);
    bysValue_[len] = 0;
    return true;
  }

  if (dv.GetDataLength() > maxLength_) {
    _threadErrorMsg.reset(
        new ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {to_string(maxLength_), to_string(dv.GetDataLength())}));
    return false;
  }

  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (bMove && dv.GetDataType() == DataType::VARCHAR) {
    bysValue_ = ((DataValueVarChar &)dv).bysValue_;
    valType_ = dv.GetValueType();
    soleLength_ = dv.GetDataLength();
    ((DataValueVarChar &)dv).bysValue_ = nullptr;
    ((DataValueVarChar &)dv).valType_ = ValueType::NULL_VALUE;
  } else if (dv.GetDataType() == DataType::VARCHAR &&
             dv.GetValueType() == ValueType::BYTES_VALUE) {
    bysValue_ = ((DataValueVarChar &)dv).bysValue_;
    valType_ = ValueType::BYTES_VALUE;
    soleLength_ = dv.GetDataLength();
  } else {
    valType_ = ValueType::SOLE_VALUE;
    if (dv.GetDataType() == DataType::FIXCHAR) {
      uint32_t len = dv.GetDataLength();
      Byte *bys = dv.GetBuff();
      while (true) {
        len--;
        if (bys[len] == ' ')
          break;
        if (len == 0)
          break;
      }

      soleLength_ = len + 1;
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, ((DataValueVarChar &)dv).bysValue_, len);
      bysValue_[len] = 0;
    } else {
      soleLength_ = dv.GetDataLength();
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, ((DataValueVarChar &)dv).bysValue_, soleLength_);
    }
  }

  return true;
}

uint32_t DataValueVarChar::WriteData(Byte *buf, SavePosition svPos) const {
  if (svPos == SavePosition::KEY) {
    // Write default value if is null for key
    if (valType_ == ValueType::NULL_VALUE) {
      *buf = 0;
      return 1;
    }

    BytesCopy(buf, bysValue_, soleLength_);
    return soleLength_;
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      return 0;
    } else {
      BytesCopy(buf, bysValue_, soleLength_);
      return soleLength_;
    }
  }
}

uint32_t DataValueVarChar::ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                                    bool bSole) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (svPos == SavePosition::KEY) {
    assert(len > 0);
    soleLength_ = len;
    if (bSole) {
      valType_ = ValueType::SOLE_VALUE;
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, buf, len);
    } else {
      valType_ = ValueType::BYTES_VALUE;
      bysValue_ = buf;
    }

    return len;
  } else {
    if (len == 0) {
      valType_ = ValueType::NULL_VALUE;
      bysValue_ = nullptr;
      return 0;
    }

    assert(len <= maxLength_);
    soleLength_ = len;
    if (bSole) {
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, buf, soleLength_);
      valType_ = ValueType::SOLE_VALUE;
    } else {
      bysValue_ = buf;
      valType_ = ValueType::BYTES_VALUE;
    }
    return soleLength_;
  }
}

uint32_t DataValueVarChar::WriteData(Byte *buf) const {
  if (valType_ == ValueType::NULL_VALUE) {
    buf[0] = (Byte)DataType::VARCHAR & DATE_TYPE;
    return 1;
  } else {
    buf[0] = (VALUE_TYPE | ((Byte)DataType::VARCHAR & DATE_TYPE));
    BytesCopy(buf + 1, (Byte *)&soleLength_, sizeof(uint32_t));
    BytesCopy(buf + 1 + sizeof(uint32_t), (char *)bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueVarChar::ReadData(Byte *buf) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }
  valType_ =
      (buf[0] & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    valType_ = ValueType::NULL_VALUE;
    return 1;
  }

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy((char *)&soleLength_, buf + 1, sizeof(uint32_t));
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, buf + 1 + sizeof(uint32_t), soleLength_);
  return soleLength_ + sizeof(uint32_t) + 1;
}

void DataValueVarChar::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = 0;
}

void DataValueVarChar::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 4;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueVarChar::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = 0;
}

DataValueVarChar &DataValueVarChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val) + 1;
  if (len >= maxLength_ - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val, soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const MString val) {
  uint32_t len = (uint32_t)val.size() + 1;
  if (len >= maxLength_ - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val.c_str(), soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const string val) {
  uint32_t len = (uint32_t)val.size() + 1;
  if (len >= maxLength_ - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val.c_str(), soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const DataValueVarChar &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release((Byte *)bysValue_, soleLength_);

  dataType_ = src.dataType_;
  valType_ = src.valType_;
  savePos_ = src.savePos_;
  maxLength_ = src.maxLength_;
  soleLength_ = src.soleLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::Apply(soleLength_);
    BytesCopy(bysValue_, src.bysValue_, soleLength_);
    break;
  case ValueType::BYTES_VALUE:
    bysValue_ = src.bysValue_;
    break;
  case ValueType::NULL_VALUE:
  default:
    bysValue_ = nullptr;
    break;
  }

  return *this;
}

std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv) {
  switch (dv.valType_) {
  case ValueType::NULL_VALUE:
    os << "nullptr";
    break;
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    os << (char *)dv.bysValue_;
    break;
  }

  return os;
}

} // namespace storage
