#include "DataValueFixChar.h"
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include <cstring>
#include <stdexcept>

namespace storage {
DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::NULL_VALUE, bKey),
      maxLength_(maxLength), bysValue_(nullptr) {}

DataValueFixChar::DataValueFixChar(const char *val, uint32_t strLen,
                                   uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength) {
  if (strLen + 1 >= maxLength_) {
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(strLen)});
  }

  bysValue_ = CachePool::Apply(maxLength_);
  BytesCopy(bysValue_, val, strLen);
  memset(bysValue_ + strLen, ' ', maxLength - strLen - 1);
  bysValue_[maxLength_ - 1] = 0;
}

DataValueFixChar::DataValueFixChar(Byte *byArray, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::BYTES_VALUE, bKey),
      bysValue_(byArray), maxLength_(maxLength) {}

DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey, std::any val)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength) {
  MString str;
  if (val.type() == typeid(string))
    str = std::any_cast<string>(val).c_str();
  else if (val.type() == typeid(MString))
    str = std::any_cast<MString>(val);
  else if (val.type() == typeid(const char *))
    str = any_cast<const char *>(val);
  else if (val.type() == typeid(char *))
    str = any_cast<char *>(val);
  else if (val.type() == typeid(int64_t))
    str = move(ToMString(any_cast<int64_t>(val)));
  else if (val.type() == typeid(int64_t))
    str = move(ToMString(any_cast<int64_t>(val)));
  else if (val.type() == typeid(int32_t))
    str = move(ToMString(any_cast<int32_t>(val)));
  else if (val.type() == typeid(int16_t))
    str = move(ToMString(any_cast<int16_t>(val)));
  else if (val.type() == typeid(uint64_t))
    str = move(ToMString(any_cast<uint64_t>(val)));
  else if (val.type() == typeid(uint32_t))
    str = move(ToMString(any_cast<uint32_t>(val)));
  else if (val.type() == typeid(uint16_t))
    str = move(ToMString(any_cast<uint16_t>(val)));
  else if (val.type() == typeid(int8_t))
    str = move(ToMString(any_cast<int8_t>(val)));
  else if (val.type() == typeid(uint8_t))
    str = move(ToMString(any_cast<uint8_t>(val)));
  else
    throw ErrorMsg(DT_UNSUPPORT_CONVERT,
                   {val.type().name(), "DataValueFixChar"});

  uint32_t len = (uint32_t)str.size();
  if (len + 1 >= maxLength_)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(len)});

  bysValue_ = CachePool::Apply(maxLength_);
  BytesCopy(bysValue_, str.c_str(), len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
}

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

DataValueFixChar::~DataValueFixChar() {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, maxLength_);
  }
}

void DataValueFixChar::Copy(const IDataValue &dv, bool bMove) {
  if (dv.IsNull()) {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }
    bysValue_ = nullptr;
    valType_ = ValueType::NULL_VALUE;
    return;
  }

  if (dataType_ != dv.GetDataType()) {
    StrBuff sb(0);
    dv.ToString(sb);
    if (maxLength_ < sb.GetStrLen() + 1) {
      throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {to_string(maxLength_), to_string(dv.GetDataLength())});
    }

    if (valType_ != ValueType::SOLE_VALUE) {
      bysValue_ = CachePool::Apply(maxLength_);
    }

    int len = sb.GetStrLen();
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, sb.GetBuff(), len);
    memset(bysValue_ + len, ' ', maxLength_ - len - 1);
    bysValue_[maxLength_ - 1] = 0;
    return;
  }

  if (dv.GetDataLength() > maxLength_) {
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(dv.GetDataLength())});
  }

  if (dv.GetValueType() == ValueType::BYTES_VALUE) {
    assert(maxLength_ == dv.GetMaxLength());
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }

    bysValue_ = ((DataValueFixChar &)dv).bysValue_;
    valType_ = ValueType::BYTES_VALUE;
  } else if (bMove) {
    assert(maxLength_ == dv.GetMaxLength());
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, maxLength_);
    }
    bysValue_ = ((DataValueFixChar &)dv).bysValue_;
    valType_ = ValueType::SOLE_VALUE;
    ((DataValueFixChar &)dv).bysValue_ = nullptr;
    ((DataValueFixChar &)dv).valType_ = ValueType::NULL_VALUE;
  } else {
    if (valType_ != ValueType::SOLE_VALUE) {
      bysValue_ = CachePool::Apply(maxLength_);
    }
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, ((DataValueFixChar &)dv).bysValue_, dv.GetMaxLength());
    if (maxLength_ > dv.GetMaxLength()) {
      memset(bysValue_ + dv.GetMaxLength() - 1, ' ',
             maxLength_ - dv.GetMaxLength());
      bysValue_[maxLength_ - 1] = 0;
    }
  }
}

std::any DataValueFixChar::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return MString((char *)bysValue_, maxLength_ - 1);
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueFixChar::WriteData(Byte *buf, bool key) const {
  if (key) {
    if (valType_ == ValueType::NULL_VALUE) {
      memset(buf, 0, maxLength_);
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

uint32_t DataValueFixChar::WriteData(fstream &fs) const {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::FIXCHAR & DATE_TYPE);
    return 1;
  } else {
    fs.put((char)(VALUE_TYPE | ((Byte)DataType::FIXCHAR & DATE_TYPE)));
    fs.write((char *)bysValue_, maxLength_);
    return maxLength_ + 1;
  }
}

uint32_t DataValueFixChar::ReadData(fstream &fs) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, maxLength_);
  }

  char c;
  fs.get(c);
  valType_ = (c & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
    return 1;
  }
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(maxLength_);
  fs.read((char *)bysValue_, maxLength_);
  return maxLength_ + 1;
}

uint32_t DataValueFixChar::ReadData(Byte *buf, uint32_t len, bool bSole) {
  assert(len == 0 || len == maxLength_);
  if (bKey_) {
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

void DataValueFixChar::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(maxLength_);
  memset(bysValue_, 0, maxLength_);
}

void DataValueFixChar::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(maxLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueFixChar::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(maxLength_);
  bysValue_[0] = 0;
}

DataValueFixChar &DataValueFixChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val);
  if (len + 1 >= maxLength_)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(len)});
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val, len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return *this;
}

DataValueFixChar &DataValueFixChar::operator=(const MString val) {
  uint32_t len = (uint32_t)val.size();
  if (len + 1 >= maxLength_)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(len)});
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val.c_str(), len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return *this;
}

DataValueFixChar &DataValueFixChar::operator=(const string val) {
  uint32_t len = (uint32_t)val.size();
  if (len + 1 >= maxLength_)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(len)});
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::Apply(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy(bysValue_, val.c_str(), len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return *this;
}

DataValueFixChar &DataValueFixChar::operator=(const DataValueFixChar &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, maxLength_);

  dataType_ = src.dataType_;
  valType_ = src.valType_;
  bKey_ = src.bKey_;
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

  return *this;
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
