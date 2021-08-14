#include "DataValueFixChar.h"
#include "../config/ErrorID.h"
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
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(strLen)});
  }

  bysValue_ = CachePool::ApplyBys(maxLength_);
  memcpy(bysValue_, val, strLen);
  memset(bysValue_ + strLen, ' ', maxLength - strLen - 1);
  bysValue_[maxLength_ - 1] = 0;
}

DataValueFixChar::DataValueFixChar(Byte *byArray, uint32_t maxLength, bool bKey)
    : IDataValue(DataType::FIXCHAR, ValueType::BYTES_VALUE, bKey),
      bysValue_(byArray), maxLength_(maxLength) {}

DataValueFixChar::DataValueFixChar(uint32_t maxLength, bool bKey, std::any val)
    : IDataValue(DataType::FIXCHAR, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength) {
  string str;
  if (val.type() == typeid(std::string))
    str = std::any_cast<std::string>(val);
  else if (val.type() == typeid(const char *))
    str = any_cast<const char *>(val);
  else if (val.type() == typeid(char *))
    str = any_cast<char *>(val);
  else if (val.type() == typeid(int64_t))
    str = move(to_string(any_cast<int64_t>(val)));
  else if (val.type() == typeid(int64_t))
    str = move(to_string(any_cast<int64_t>(val)));
  else if (val.type() == typeid(int32_t))
    str = move(to_string(any_cast<int32_t>(val)));
  else if (val.type() == typeid(int16_t))
    str = move(to_string(any_cast<int16_t>(val)));
  else if (val.type() == typeid(uint64_t))
    str = move(to_string(any_cast<uint64_t>(val)));
  else if (val.type() == typeid(uint32_t))
    str = move(to_string(any_cast<uint32_t>(val)));
  else if (val.type() == typeid(uint16_t))
    str = move(to_string(any_cast<uint16_t>(val)));
  else if (val.type() == typeid(int8_t))
    str = move(to_string(any_cast<int8_t>(val)));
  else if (val.type() == typeid(uint8_t))
    str = move(to_string(any_cast<uint8_t>(val)));
  else
    throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT,
                          {val.type().name(), "DataValueFixChar"});

  uint32_t len = (uint32_t)str.size();
  if (len + 1 >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(len)});

  bysValue_ = CachePool::ApplyBys(maxLength_);
  memcpy(bysValue_, str.c_str(), len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
}

DataValueFixChar::DataValueFixChar(const DataValueFixChar &src)
    : IDataValue(src) {
  maxLength_ = src.maxLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::ApplyBys(maxLength_);
    memcpy(bysValue_, src.bysValue_, maxLength_);
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
    CachePool::ReleaseBys(bysValue_, maxLength_);
  }
}

DataValueFixChar *DataValueFixChar::CloneDataValue(bool incVal) {
  if (incVal) {
    return new DataValueFixChar(*this);
  } else {
    return new DataValueFixChar(maxLength_, bKey_);
  }
}

std::any DataValueFixChar::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return string((char *)bysValue_);
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueFixChar::GetPersistenceLength(bool key) const {
  if (key) {
    return maxLength_;
  } else {
    switch (valType_) {
    case ValueType::SOLE_VALUE:
      return maxLength_ + 1;
    case ValueType::NULL_VALUE:
    default:
      return 1;
    }
  }
}

uint32_t DataValueFixChar::WriteData(Byte *buf, bool key) {
  if (key) {
    assert(valType_ != ValueType::NULL_VALUE);
    std::memcpy(buf, bysValue_, maxLength_);

    return maxLength_;
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      *buf = (Byte)DataType::FIXCHAR & DATE_TYPE;
      return 1;
    } else {
      *buf = VALUE_TYPE | ((Byte)DataType::FIXCHAR & DATE_TYPE);
      buf++;
      std::memcpy(buf, bysValue_, maxLength_);
      return maxLength_ + 1;
    }
  }
}

uint32_t DataValueFixChar::WriteData(fstream &fs) {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::FIXCHAR & DATE_TYPE);
    return 1;
  } else {
    fs.put(VALUE_TYPE | ((Byte)DataType::FIXCHAR & DATE_TYPE));
    fs.write((char *)bysValue_, maxLength_);
    return maxLength_ + 1;
  }
}

uint32_t DataValueFixChar::ReadData(fstream &fs) {
  char c;
  fs.get(c);

  valType_ = (c & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE)
    return 1;

  bysValue_ = CachePool::ApplyBys(maxLength_);
  fs.read((char *)bysValue_, maxLength_);
  return maxLength_ + 1;
}

uint32_t DataValueFixChar::ReadData(Byte *buf, uint32_t len, bool bSole) {
  if (bKey_) {
    valType_ = ValueType::SOLE_VALUE;
    bysValue_ = CachePool::ApplyBys(maxLength_);
    memcpy(bysValue_, buf, maxLength_);
    return maxLength_;
  } else {
    valType_ =
        (*buf & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
    buf++;

    if (valType_ == ValueType::NULL_VALUE)
      return 1;

    bysValue_ = CachePool::ApplyBys(maxLength_);
    memcpy(bysValue_, buf, maxLength_);
    return maxLength_ + 1;
  }
}

uint32_t DataValueFixChar::GetDataLength() const {
  if (!bKey_ && valType_ == ValueType::NULL_VALUE)
    return 0;
  else
    return maxLength_;
}

uint32_t DataValueFixChar::GetMaxLength() const { return maxLength_; }

uint32_t DataValueFixChar::GetPersistenceLength() const {
  if (bKey_) {
    return maxLength_;
  } else {
    switch (valType_) {
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

void DataValueFixChar::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::ApplyBys(maxLength_);
  memset(bysValue_, 0, maxLength_);
}

void DataValueFixChar::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::ApplyBys(maxLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueFixChar::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::ApplyBys(maxLength_);
  bysValue_[0] = 0;
}

DataValueFixChar::operator string() const {
  switch (valType_) {
  case ValueType::NULL_VALUE:
    return "";
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return string((char *)bysValue_);
  }

  return "";
}

DataValueFixChar &DataValueFixChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val) + 1;
  if (len >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(len)});
  if (valType_ != ValueType::SOLE_VALUE)
    bysValue_ = CachePool::ApplyBys(maxLength_);

  valType_ = ValueType::SOLE_VALUE;
  memcpy(bysValue_, val, len);
  memset(bysValue_ + len, ' ', maxLength_ - len - 1);
  bysValue_[maxLength_ - 1] = 0;
  return *this;
}

DataValueFixChar &DataValueFixChar::operator=(const DataValueFixChar &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, maxLength_);

  dataType_ = src.dataType_;
  valType_ = src.valType_;
  bKey_ = src.bKey_;
  maxLength_ = src.maxLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::ApplyBys(maxLength_);
    memcpy(bysValue_, src.bysValue_, maxLength_);
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

bool DataValueFixChar::operator>(const DataValueFixChar &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return false;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return true;
  }

  int rt = strncmp((char *)bysValue_, (char *)dv.bysValue_,
                   std::min(GetDataLength(), dv.GetDataLength()));
  if (rt == 0) {
    return GetDataLength() > dv.GetDataLength();
  } else {
    return rt > 0;
  }
}

bool DataValueFixChar::operator<(const DataValueFixChar &dv) const {
  return !(*this >= dv);
}

bool DataValueFixChar::operator>=(const DataValueFixChar &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return true;
  }

  int rt = strncmp((char *)bysValue_, (char *)dv.bysValue_,
                   std::min(GetDataLength(), dv.GetDataLength()));
  if (rt == 0) {
    return GetDataLength() >= dv.GetDataLength();
  } else {
    return rt > 0;
  }
}

bool DataValueFixChar::operator<=(const DataValueFixChar &dv) const {
  return !(*this > dv);
}

bool DataValueFixChar::operator==(const DataValueFixChar &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return false;
  }

  return strncmp((char *)bysValue_, (char *)dv.bysValue_,
                 std::min(GetDataLength(), dv.GetDataLength())) == 0;
}

bool DataValueFixChar::operator!=(const DataValueFixChar &dv) const {
  return !(*this == dv);
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

void DataValueFixChar::ToString(StrBuff &sb) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return;
  }

  sb.Cat((char *)bysValue_, maxLength_ - 1);
}
} // namespace storage
