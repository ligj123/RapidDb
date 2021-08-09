#include "DataValueVarChar.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include <cstring>
#include <stdexcept>

namespace storage {
DataValueVarChar::DataValueVarChar(uint32_t maxLength, bool bKey)
    : IDataValue(DataType::VARCHAR, ValueType::NULL_VALUE, bKey),
      maxLength_(maxLength), soleLength_(0), bysValue_(nullptr) {}

DataValueVarChar::DataValueVarChar(const char *val, uint32_t strLen,
                                   uint32_t maxLength, bool bKey)
    : IDataValue(DataType::VARCHAR, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength), soleLength_(strLen + 1) {
  if (soleLength_ >= maxLength_) {
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});
  }

  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, val, soleLength_);
}

DataValueVarChar::DataValueVarChar(Byte *byArray, uint32_t strLen,
                                   uint32_t maxLength, bool bKey)
    : IDataValue(DataType::VARCHAR, ValueType::BYTES_VALUE, bKey),
      bysValue_(byArray), maxLength_(maxLength), soleLength_(strLen) {}

DataValueVarChar::DataValueVarChar(uint32_t maxLength, bool bKey, std::any val)
    : IDataValue(DataType::VARCHAR, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength) {
  string str;
  if (val.type() == typeid(std::string))
    str = std::any_cast<std::string>(val);
  else if (val.type() == typeid(const char *))
    str = any_cast<const char *>(val);
  else if (val.type() == typeid(char *))
    str = any_cast<char *>(val);
  else if (val.type() == typeid(int64_t))
    str = move(to_string(std::any_cast<int64_t>(val)));
  else if (val.type() == typeid(int64_t))
    str = move(to_string(std::any_cast<int64_t>(val)));
  else if (val.type() == typeid(int32_t))
    str = move(to_string(std::any_cast<int32_t>(val)));
  else if (val.type() == typeid(int16_t))
    str = move(to_string(std::any_cast<int16_t>(val)));
  else if (val.type() == typeid(uint64_t))
    str = move(to_string(std::any_cast<uint64_t>(val)));
  else if (val.type() == typeid(uint32_t))
    str = move(to_string(std::any_cast<uint32_t>(val)));
  else if (val.type() == typeid(uint16_t))
    str = move(to_string(std::any_cast<uint16_t>(val)));
  else if (val.type() == typeid(int8_t))
    str = move(to_string(std::any_cast<int8_t>(val)));
  else if (val.type() == typeid(uint8_t))
    str = move(to_string(std::any_cast<uint8_t>(val)));
  else
    throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT,
                          {val.type().name(), "DataValueVarChar"});

  soleLength_ = (uint32_t)str.size() + 1;
  if (soleLength_ >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});

  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, str.c_str(), soleLength_);
}

DataValueVarChar::DataValueVarChar(const DataValueVarChar &src)
    : IDataValue(src) {
  maxLength_ = src.maxLength_;
  soleLength_ = src.soleLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::ApplyBys(soleLength_);
    memcpy(bysValue_, src.bysValue_, soleLength_);
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

DataValueVarChar::~DataValueVarChar() {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::ReleaseBys((Byte *)bysValue_, soleLength_);
    valType_ = ValueType::NULL_VALUE;
  }
}

DataValueVarChar *DataValueVarChar::CloneDataValue(bool incVal) {
  if (incVal) {
    return new DataValueVarChar(*this);
  } else {
    return new DataValueVarChar(maxLength_, bKey_);
  }
}

std::any DataValueVarChar::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return string((char *)bysValue_);
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueVarChar::WriteData(Byte *buf) {
  return WriteData(buf, bKey_);
}

uint32_t DataValueVarChar::GetPersistenceLength(bool key) const {
  if (key) {
    return soleLength_;
  } else {
    switch (valType_) {
    case ValueType::SOLE_VALUE:
      return soleLength_ + 1 + sizeof(uint32_t);
    case ValueType::NULL_VALUE:
    default:
      return 1;
    }
  }
}

uint32_t DataValueVarChar::WriteData(Byte *buf, bool key) {
  if (bKey_) {
    assert(valType_ != ValueType::NULL_VALUE);
    std::memcpy(buf, bysValue_, soleLength_);
    return soleLength_;
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      *buf = (Byte)DataType::VARCHAR & DATE_TYPE;
      return 1;
    } else if (valType_ == ValueType::BYTES_VALUE) {
      *buf = VALUE_TYPE | ((Byte)DataType::VARCHAR & DATE_TYPE);
      buf++;
      *((uint32_t *)buf) = soleLength_;
      buf += sizeof(uint32_t);
      std::memcpy(buf, bysValue_, soleLength_);
      return soleLength_ + sizeof(uint32_t) + 1;
    } else {
      *buf = VALUE_TYPE | ((Byte)DataType::VARCHAR & DATE_TYPE);
      buf++;
      *((uint32_t *)buf) = soleLength_;
      buf += sizeof(uint32_t);
      std::memcpy(buf, bysValue_, soleLength_);
      return (uint32_t)soleLength_ + sizeof(uint32_t) + 1;
    }
  }
}

uint32_t DataValueVarChar::WriteData(fstream &fs) {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::VARCHAR & DATE_TYPE);
    return 1;
  } else {
    fs.put(VALUE_TYPE | ((Byte)DataType::VARCHAR & DATE_TYPE));
    fs.write((char *)&soleLength_, sizeof(uint32_t));
    fs.write((char *)bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueVarChar::ReadData(fstream &fs) {
  char c;
  fs.get(c);

  valType_ = (c & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE)
    return 1;

  fs.read((char *)&soleLength_, sizeof(uint32_t));
  bysValue_ = CachePool::ApplyBys(soleLength_);
  fs.read((char *)bysValue_, soleLength_);
  return soleLength_ + sizeof(uint32_t) + 1;
}

uint32_t DataValueVarChar::ReadData(Byte *buf, uint32_t len, bool bSole) {
  if (bKey_) {
    assert(len > 0);
    valType_ = ValueType::SOLE_VALUE;
    soleLength_ = len;
    bysValue_ = CachePool::ApplyBys(soleLength_);
    memcpy(bysValue_, buf, len);
    return len;
  } else {
    valType_ =
        (*buf & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
    buf++;

    if (valType_ == ValueType::NULL_VALUE)
      return 1;

    soleLength_ = *((uint32_t *)buf);
    buf += sizeof(uint32_t);
    bysValue_ = CachePool::ApplyBys(soleLength_);
    memcpy(bysValue_, buf, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueVarChar::GetDataLength() const {
  if (!bKey_ && valType_ == ValueType::NULL_VALUE)
    return 0;
  else
    return soleLength_;
}

uint32_t DataValueVarChar::GetMaxLength() const { return maxLength_; }

uint32_t DataValueVarChar::GetPersistenceLength() const {
  if (bKey_) {
    return soleLength_;
  } else {
    switch (valType_) {
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
void DataValueVarChar::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  bysValue_[0] = 0;
}

void DataValueVarChar::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 4;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueVarChar::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  bysValue_[0] = 0;
}

DataValueVarChar::operator string() const {
  switch (valType_) {
  case ValueType::NULL_VALUE:
    return "";
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return string((char *)bysValue_);
  }

  return "";
}

DataValueVarChar &DataValueVarChar::operator=(char *val) {
  uint32_t len = (uint32_t)strlen(val) + 1;
  if (len >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = len;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, val, soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val) + 1;
  if (len >= maxLength_ - 1)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, val, soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const DataValueVarChar &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys((Byte *)bysValue_, soleLength_);

  dataType_ = src.dataType_;
  valType_ = src.valType_;
  bKey_ = src.bKey_;
  maxLength_ = src.maxLength_;
  soleLength_ = src.soleLength_;

  switch (valType_) {
  case ValueType::SOLE_VALUE:
    bysValue_ = CachePool::ApplyBys(soleLength_);
    memcpy(bysValue_, src.bysValue_, soleLength_);
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

bool DataValueVarChar::operator>(const DataValueVarChar &dv) const {
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

bool DataValueVarChar::operator<(const DataValueVarChar &dv) const {
  return !(*this >= dv);
}

bool DataValueVarChar::operator>=(const DataValueVarChar &dv) const {
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

bool DataValueVarChar::operator<=(const DataValueVarChar &dv) const {
  return !(*this > dv);
}

bool DataValueVarChar::operator==(const DataValueVarChar &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return false;
  }

  if (GetDataLength() != dv.GetDataLength())
    return false;

  return strncmp((char *)bysValue_, (char *)dv.bysValue_,
                 std::min(GetDataLength(), dv.GetDataLength())) == 0;
}

bool DataValueVarChar::operator!=(const DataValueVarChar &dv) const {
  return !(*this == dv);
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

void DataValueVarChar::ToString(StrBuff &sb) {
  if (valType_ == ValueType::NULL_VALUE) {
    return;
  }

  sb.Cat((char *)bysValue_, soleLength_ - 1);
}
} // namespace storage
