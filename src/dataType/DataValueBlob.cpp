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
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  }

  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val, soleLength_);
}

DataValueBlob::DataValueBlob(Byte *byArray, uint32_t len, uint32_t maxLength,
                             bool bKey)
    : IDataValue(DataType::BLOB, ValueType::BYTES_VALUE, bKey),
      bysValue_(byArray), maxLength_(maxLength), soleLength_(len) {}

DataValueBlob::DataValueBlob(MVector<Byte>::Type &vct, uint32_t maxLength,
                             bool bKey)
    : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, bKey),
      maxLength_(maxLength), soleLength_(vct.size()) {
  if (soleLength_ > maxLength_) {
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  }

  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, vct.data(), soleLength_);
}

DataValueBlob::DataValueBlob(const DataValueBlob &src) : IDataValue(src) {
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
}

DataValueBlob::~DataValueBlob() {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
    valType_ = ValueType::NULL_VALUE;
  }
}

void DataValueBlob::Copy(const IDataValue &dv, bool bMove) {
  if (dataType_ != dv.GetDataType()) {
    throw ErrorMsg(DT_UNSUPPORT_CONVERT,
                   {StrOfDataType(dv.GetDataType()), StrOfDataType(dataType_)});
  }

  if (dv.GetDataLength() > maxLength_) {
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(dv.GetDataLength())});
  }

  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (dv.GetValueType() == ValueType::BYTES_VALUE) {
    bysValue_ = ((DataValueBlob &)dv).bysValue_;
    valType_ = ValueType::BYTES_VALUE;
    soleLength_ = dv.GetDataLength();
  } else if (bMove) {
    bysValue_ = ((DataValueBlob &)dv).bysValue_;
    valType_ = dv.GetValueType();
    soleLength_ = dv.GetDataLength();
    ((DataValueBlob &)dv).bysValue_ = nullptr;
    ((DataValueBlob &)dv).valType_ = ValueType::NULL_VALUE;
  } else if (dv.GetValueType() != ValueType::NULL_VALUE) {
    bysValue_ = CachePool::Apply(soleLength_);
    valType_ = ValueType::SOLE_VALUE;
    soleLength_ = dv.GetDataLength();
    BytesCopy(bysValue_, ((DataValueBlob &)dv).bysValue_, soleLength_);
  } else {
    valType_ = ValueType::NULL_VALUE;
    soleLength_ = 0;
    bysValue_ = nullptr;
  }
}

uint32_t DataValueBlob::WriteData(Byte *buf, bool key) const {
  assert(!key);

  if (valType_ == ValueType::NULL_VALUE) {
    return 0;
  } else {
    BytesCopy(buf, bysValue_, soleLength_);
    return soleLength_;
  }
}

uint32_t DataValueBlob::ReadData(Byte *buf, uint32_t len, bool bSole) {
  assert(!bKey_);
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (len == 0) {
    valType_ = ValueType::NULL_VALUE;
    return 1;
  }

  soleLength_ = len;
  if (bSole) {
    bysValue_ = CachePool::Apply(soleLength_);
    BytesCopy(bysValue_, buf, soleLength_);
    valType_ = ValueType::SOLE_VALUE;
  } else {
    valType_ = ValueType::BYTES_VALUE;
    bysValue_ = buf;
  }

  return soleLength_;
}

uint32_t DataValueBlob::WriteData(fstream &fs) const {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::BLOB & DATE_TYPE);
    return 1;
  } else {
    fs.put((char)(VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE)));
    fs.write((char *)&soleLength_, sizeof(uint32_t));
    fs.write((char *)bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueBlob::ReadData(fstream &fs) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  Byte by;
  fs.read((char *)&by, 1);
  valType_ =
      ((by & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    return 1;
  }

  valType_ = ValueType::SOLE_VALUE;
  fs.read((char *)&soleLength_, sizeof(uint32_t));
  bysValue_ = CachePool::Apply(soleLength_);
  fs.read((char *)bysValue_, soleLength_);
  return soleLength_ + sizeof(uint32_t) + 1;
}

void DataValueBlob::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = 0;
}

void DataValueBlob::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 4;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = bysValue_[3] = 0xff;
}

void DataValueBlob::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::Apply(soleLength_);
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

void DataValueBlob::Put(uint32_t len, char *val) {
  if (len >= maxLength_)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val, soleLength_);
}

DataValueBlob &DataValueBlob::operator=(const DataValueBlob &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = src.valType_;
  bKey_ = src.bKey_;
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

  return BytesCompare(bysValue_, len, dv.bysValue_, len) == 0;
}

void DataValueBlob::ToString(StrBuff &sb) const {
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
