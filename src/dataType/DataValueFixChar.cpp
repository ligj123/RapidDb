#include "DataValueFixChar.h"
#include "../config/ErrorID.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace storage {
DataValueFixChar::DataValueFixChar(const DataValueFixChar &src)
    : IDataValue(src) {
  maxLength_ = src.maxLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::Apply(maxLength_);
    BytesCopy(bysValue_, src.bysValue_, maxLength_);
    break;
  case ValueType::BYTES_VALUE:
    bysValue_ = src.bysValue_;
    break;
  case ValueType::NULL_VALUE:
  default:
    bysValue_ = nullptr;
    break;
  }
}

bool DataValueFixChar::SetValue(const char *val, uint32_t len) {
  if (len >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return false;
  }

  if (valType_ != ValueType::SOLE_VALUE) {
    bysValue_ = CachePool::Apply(maxLength_);
  }

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val, len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return true;
}

bool DataValueFixChar::PutValue(std::any val) {
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
    abort();
  }

  if (len == 0)
    len = strlen(buf);
  if (len + 1 > maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return false;
  }

  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, buf, len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return true;
}

bool DataValueFixChar::Copy(const IDataValue &dv, bool bMove) {
  if (dv.IsNull()) {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }
    bysValue_ = nullptr;
    valType_ = ValueType::NULL_VALUE;
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

    if (valType_ != ValueType::SOLE_VALUE) {
      bysValue_ = CachePool::Apply(maxLength_);
    }

    int len = sb.GetStrLen();
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, sb.GetBuff(), len);
    memset(bysValue_ + len, ' ', maxLength_ - len - 1);
    bysValue_[maxLength_ - 1] = 0;
    return true;
  }

  if (dv.GetDataLength() > maxLength_) {
    _threadErrorMsg.reset(
        new ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {to_string(maxLength_), to_string(dv.GetDataLength())}));
    return false;
  }

  if (bMove && dv.GetDataType() == DataType::FIXCHAR &&
      maxLength_ == dv.GetMaxLength()) {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }
    bysValue_ = ((DataValueFixChar &)dv).bysValue_;
    valType_ = dv.GetValueType();
    ((DataValueFixChar &)dv).bysValue_ = nullptr;
    ((DataValueFixChar &)dv).valType_ = ValueType::NULL_VALUE;
  } else if (dv.GetValueType() == ValueType::BYTES_VALUE &&
             maxLength_ == dv.GetMaxLength()) {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }

    bysValue_ = ((DataValueFixChar &)dv).bysValue_;
    valType_ = ValueType::BYTES_VALUE;
  } else {
    if (valType_ != ValueType::SOLE_VALUE) {
      bysValue_ = CachePool::Apply(maxLength_);
    }
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, ((DataValueFixChar &)dv).bysValue_,
              dv.GetMaxLength() - 1);
    if (maxLength_ > dv.GetMaxLength()) {
      memset(bysValue_ + dv.GetMaxLength() - 1, ' ',
             maxLength_ - dv.GetMaxLength());
    }

    bysValue_[maxLength_ - 1] = 0;
  }
  return true;
}

uint32_t DataValueFixChar::WriteData(Byte *buf, SavePosition svPos) const {
  if (svPos == SavePosition::KEY) {
    if (valType_ == ValueType::NULL_VALUE) {
      memset(buf, ' ', maxLength_ - 1);
      buf[maxLength_ - 1] = 0;
    } else {
      BytesCopy(buf, bysValue_, maxLength_);
    }
    return maxLength_;
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      return 0;
    } else {
      BytesCopy(buf, bysValue_, maxLength_);
      return maxLength_;
    }
  }
}

uint32_t DataValueFixChar::ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                                    bool bSole) {
  assert(len == 0 || len == maxLength_);
  if (svPos == SavePosition::KEY) {
    assert(len > 0);
    if (bSole) {
      if (valType_ != ValueType::SOLE_VALUE) {
        bysValue_ = CachePool::Apply(maxLength_);
      }
      valType_ = ValueType::SOLE_VALUE;
      BytesCopy(bysValue_, buf, maxLength_);
    } else {
      if (valType_ == ValueType::SOLE_VALUE) {
        CachePool::Release(bysValue_, maxLength_);
      }
      valType_ = ValueType::BYTES_VALUE;
      bysValue_ = buf;
    }
    return maxLength_;
  } else {
    if (len == 0) {
      if (valType_ == ValueType::SOLE_VALUE) {
        CachePool::Release(bysValue_, maxLength_);
      }
      valType_ = ValueType::NULL_VALUE;
      return 0;
    }

    if (bSole) {
      if (valType_ != ValueType::SOLE_VALUE)
        bysValue_ = CachePool::Apply(maxLength_);
      valType_ = ValueType::SOLE_VALUE;
      BytesCopy(bysValue_, buf, maxLength_);
    } else {
      if (valType_ == ValueType::SOLE_VALUE)
        CachePool::Release(bysValue_, maxLength_);
      valType_ = ValueType::BYTES_VALUE;
      bysValue_ = buf;
    }

    return maxLength_;
  }
}

uint32_t DataValueFixChar::WriteData(Byte *buf) const {
  if (valType_ == ValueType::NULL_VALUE) {
    buf[0] = (Byte)DataType::FIXCHAR & DATE_TYPE;
    return 1;
  } else {
    buf[0] = (Byte)(VALUE_TYPE | ((Byte)DataType::FIXCHAR & DATE_TYPE));
    BytesCopy(buf + 1, (Byte *)&maxLength_, sizeof(uint32_t));
    BytesCopy(buf + 1 + sizeof(uint32_t), bysValue_, maxLength_);
    return maxLength_ + 1;
  }
}

uint32_t DataValueFixChar::ReadData(Byte *buf) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, maxLength_);
  }

  valType_ =
      (buf[0] & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
    return 1;
  }

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy((Byte *)&maxLength_, buf + 1, sizeof(uint32_t));
  bysValue_ = CachePool::Apply(maxLength_);
  BytesCopy(bysValue_, buf + 1 + sizeof(uint32_t), maxLength_);
  return maxLength_ + 1;
}

void DataValueFixChar::SetMinValue() {
  if (valType_ != ValueType::SOLE_VALUE) {
    bysValue_ = CachePool::Apply(maxLength_);
  }
  valType_ = ValueType::SOLE_VALUE;
  memset(bysValue_, 0, maxLength_);
}

void DataValueFixChar::SetMaxValue() {
  if (valType_ != ValueType::SOLE_VALUE) {
    bysValue_ = CachePool::Apply(maxLength_);
  }

  valType_ = ValueType::SOLE_VALUE;
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueFixChar::SetDefaultValue() {
  if (valType_ != ValueType::SOLE_VALUE) {
    bysValue_ = CachePool::Apply(maxLength_);
  }

  valType_ = ValueType::SOLE_VALUE;
  memset(bysValue_, ' ', maxLength_ - 1);
  bysValue_[maxLength_ - 1] = 0;
}

DataValueFixChar *DataValueFixChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val);
  if (len + 1 >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return nullptr;
  }
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val, len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return this;
}

DataValueFixChar *DataValueFixChar::operator=(const MString val) {
  uint32_t len = (uint32_t)val.size();
  if (len >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return nullptr;
  }
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val.c_str(), len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return this;
}

DataValueFixChar *DataValueFixChar::operator=(const string val) {
  uint32_t len = (uint32_t)val.size();
  if (len >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return nullptr;
  }
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val.c_str(), len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return this;
}

DataValueFixChar *DataValueFixChar::operator=(const DataValueFixChar &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, maxLength_);

  dataType_ = src.dataType_;
  valType_ = src.valType_;
  maxLength_ = src.maxLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::Apply(maxLength_);
    BytesCopy(bysValue_, src.bysValue_, maxLength_);
    break;
  case ValueType::BYTES_VALUE:
    bysValue_ = src.bysValue_;
    break;
  case ValueType::NULL_VALUE:
  default:
    bysValue_ = nullptr;
    break;
  }

  return this;
}

std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv) {
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
