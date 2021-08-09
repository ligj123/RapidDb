#include "DataValueBlob.h"
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include <cstring>
#include <stdexcept>

namespace storage {
DataValueBlob::DataValueBlob(uint32_t maxLength, bool bKey)
    : IDataValue(DataType::BLOB, ValueType::NULL_VALUE, bKey),
      maxLength_(maxLength), bysValue_(nullptr), soleLength_(0) {}

DataValueBlob::DataValueBlob(const char *val, uint32_t len, uint32_t maxLength,
                             bool bKey)
    : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength), soleLength_(len) {
  if (soleLength_ > maxLength_) {
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});
  }

  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, val, soleLength_);
}

DataValueBlob::DataValueBlob(Byte *byArray, uint32_t len, uint32_t maxLength,
                             bool bKey)
    : IDataValue(DataType::BLOB, ValueType::BYTES_VALUE, bKey),
      bysValue_(byArray), maxLength_(maxLength), soleLength_(len) {}

DataValueBlob::DataValueBlob(uint32_t maxLength, bool bKey, std::any val)
    : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength) {
  throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT,
                        {val.type().name(), "DataValueBlob"});
}

DataValueBlob::DataValueBlob(const DataValueBlob &src) : IDataValue(src) {
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

DataValueBlob::~DataValueBlob() {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::ReleaseBys(bysValue_, soleLength_);
    valType_ = ValueType::NULL_VALUE;
  }
}

DataValueBlob *DataValueBlob::CloneDataValue(bool incVal) {
  if (incVal) {
    return new DataValueBlob(*this);
  } else {
    return new DataValueBlob(maxLength_, bKey_);
  }
}

std::any DataValueBlob::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return (char *)bysValue_;
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueBlob::GetPersistenceLength(bool key) const {
  if (key) {
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

uint32_t DataValueBlob::WriteData(Byte *buf, bool key) {
  assert(!bKey_);

  if (valType_ == ValueType::NULL_VALUE) {
    *buf = ((Byte)DataType::BLOB & DATE_TYPE);
    return 1;
  } else if (valType_ == ValueType::BYTES_VALUE) {
    *buf = VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE);
    buf++;
    *((uint32_t *)buf) = soleLength_;
    buf += sizeof(uint32_t);
    std::memcpy(buf, bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  } else {
    *buf = VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE);
    buf++;
    *((uint32_t *)buf) = soleLength_;
    buf += sizeof(uint32_t);
    std::memcpy(buf, bysValue_, soleLength_);
    return (uint32_t)soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueBlob::WriteData(fstream &fs) {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::BLOB & DATE_TYPE);
    return 1;
  } else {
    fs.put(VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE));
    fs.write((char *)&soleLength_, sizeof(uint32_t));
    fs.write((char *)bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueBlob::ReadData(fstream &fs) {
  Byte by;
  fs.read((char *)&by, 1);
  valType_ =
      ((by & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE)
    return 1;
  fs.read((char *)&soleLength_, sizeof(uint32_t));
  bysValue_ = CachePool::ApplyBys(soleLength_);
  fs.read((char *)bysValue_, soleLength_);
  return soleLength_ + sizeof(uint32_t) + 1;
}

uint32_t DataValueBlob::ReadData(Byte *buf, uint32_t len, bool bSole) {
  assert(!bKey_);

  valType_ = ((*buf & VALUE_TYPE)
                  ? (bSole ? ValueType::SOLE_VALUE : ValueType::BYTES_VALUE)
                  : ValueType::NULL_VALUE);
  buf++;

  if (valType_ == ValueType::NULL_VALUE)
    return 1;

  soleLength_ = *((uint32_t *)buf);
  buf += sizeof(uint32_t);
  if (valType_ == ValueType::SOLE_VALUE) {
    bysValue_ = CachePool::ApplyBys(soleLength_);
    memcpy(bysValue_, buf, soleLength_);
  } else {
    bysValue_ = buf;
  }

  return soleLength_ + sizeof(uint32_t) + 1;
}

uint32_t DataValueBlob::GetDataLength() const {
  assert(!bKey_);
  if (valType_ == ValueType::NULL_VALUE)
    return 0;
  else
    return soleLength_;
}

uint32_t DataValueBlob::GetPersistenceLength() const {
  assert(!bKey_);

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

void DataValueBlob::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  bysValue_[0] = 0;
}

void DataValueBlob::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 4;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueBlob::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  bysValue_[0] = 0;
}

DataValueBlob::operator const char *() const {
  switch (valType_) {
  case ValueType::NULL_VALUE:
    return nullptr;
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return (char *)bysValue_;
  }

  return nullptr;
}

DataValueBlob &DataValueBlob::operator=(const char *val) {
  uint32_t len = *(uint32_t *)val;
  if (len >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, val + sizeof(uint32_t), soleLength_);
  return *this;
}

void DataValueBlob::Put(uint32_t len, char *val) {
  if (len >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
                          {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::ApplyBys(soleLength_);
  memcpy(bysValue_, val, soleLength_);
}

DataValueBlob &DataValueBlob::operator=(const DataValueBlob &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::ReleaseBys(bysValue_, soleLength_);

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
    break;
  }

  return *this;
}

bool DataValueBlob::operator==(const DataValueBlob &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return false;
  }

  uint32_t len = GetDataLength();
  if (len != dv.GetDataLength())
    return false;

  return utils::BytesCompare(bysValue_, len, dv.bysValue_, len) == 0;
}

void DataValueBlob::ToString(StrBuff &sb) {
  if (valType_ == ValueType::NULL_VALUE) {
    return;
  }
  if (soleLength_ * 2 + 3 > sb.GetFreeLen()) {
    sb.Resize(sb.GetStrLen() + soleLength_ * 2 + 3);
  }

  Byte *src = bysValue_;
  char *dest = sb.GetFreeBuff();
  std::strcpy(dest, "0x");
  dest += 2;
  for (uint32_t i = 0; i < soleLength_; i++) {
    std::sprintf(dest, "%02X", *src);
    src++;
    dest += 2;
  }

  sb.SetStrLen(sb.GetStrLen() + soleLength_ * 2 + 2);
}

std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv) {
  switch (dv.valType_) {
  case ValueType::NULL_VALUE:
    os << "nullptr";
    break;
  case ValueType::SOLE_VALUE:
    os << "size=" << dv.soleLength_ << "\tValType:" << ValueType::SOLE_VALUE;
    break;
  case ValueType::BYTES_VALUE:
    os << "size=" << dv.soleLength_ << "\tValType:" << ValueType::BYTES_VALUE;
    break;
  }

  return os;
}
} // namespace storage
