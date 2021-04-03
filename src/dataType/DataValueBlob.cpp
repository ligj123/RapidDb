#include "DataValueBlob.h"
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include <cstring>
#include <stdexcept>

namespace storage {
DataValueBlob::DataValueBlob(uint32_t maxLength, bool bKey)
  : IDataValue(DataType::BLOB, ValueType::NULL_VALUE, bKey),
  maxLength_(maxLength) {}

DataValueBlob::DataValueBlob(char* val, uint32_t len, uint32_t maxLength,
  bool bKey)
  : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, bKey), soleValue_(val),
  maxLength_(maxLength), soleLength_(len) {
  if (soleLength_ > maxLength_) {
    delete[] val;
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
      { to_string(maxLength_), to_string(soleLength_) });
  }
}

DataValueBlob::DataValueBlob(const char* val, uint32_t len, uint32_t maxLength,
  bool bKey)
  : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, bKey),
  maxLength_(maxLength), soleLength_(len) {
  if (soleLength_ > maxLength_) {
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
      { to_string(maxLength_), to_string(soleLength_) });
  }

  soleValue_ = new char[soleLength_];
  memcpy(soleValue_, val, soleLength_);
}

DataValueBlob::DataValueBlob(Byte* byArray, uint32_t len, uint32_t maxLength,
  bool bKey)
  : IDataValue(DataType::BLOB, ValueType::BYTES_VALUE, bKey),
  byArray_(byArray), maxLength_(maxLength), soleLength_(len) {}

DataValueBlob::DataValueBlob(uint32_t maxLength, bool bKey, std::any val)
  : IDataValue(DataType::BLOB, ValueType::SOLE_VALUE, bKey),
  maxLength_(maxLength) {
  throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT,
    { val.type().name(), "DataValueBlob" });
}

DataValueBlob::DataValueBlob(const DataValueBlob& src) : IDataValue(src) {
  maxLength_ = src.maxLength_;
  soleLength_ = src.soleLength_;

  switch (valType_) {
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

DataValueBlob::~DataValueBlob() {
  if (valType_ == ValueType::SOLE_VALUE) {
    delete[] soleValue_;
    valType_ = ValueType::NULL_VALUE;
  }
}

DataValueBlob* DataValueBlob::CloneDataValue(bool incVal) {
  if (incVal) {
    return new DataValueBlob(*this);
  } else {
    return new DataValueBlob(maxLength_, bKey_);
  }
}

std::any DataValueBlob::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
    return soleValue_;
  case ValueType::BYTES_VALUE:
    return (char*)byArray_;
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueBlob::WriteData(Byte* buf) { return WriteData(buf, bKey_); }

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

uint32_t DataValueBlob::WriteData(Byte* buf, bool key) {
  assert(!bKey_);

  if (valType_ == ValueType::NULL_VALUE) {
    *buf = ((Byte)DataType::BLOB & DATE_TYPE);
    return 1;
  } else if (valType_ == ValueType::BYTES_VALUE) {
    *buf = VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE);
    buf++;
    *((uint32_t*)buf) = soleLength_;
    buf += sizeof(uint32_t);
    std::memcpy(buf, byArray_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  } else {
    *buf = VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE);
    buf++;
    *((uint32_t*)buf) = soleLength_;
    buf += sizeof(uint32_t);
    std::memcpy(buf, soleValue_, soleLength_);
    return (uint32_t)soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueBlob::ReadData(Byte* buf, uint32_t len) {
  assert(!bKey_);

  valType_ = ((*buf & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  buf++;

  if (valType_ == ValueType::NULL_VALUE)
    return 1;

  soleLength_ = *((uint32_t*)buf);
  buf += sizeof(uint32_t);
  soleValue_ = new char[soleLength_];
  memcpy(soleValue_, buf, soleLength_);
  return soleLength_ + sizeof(uint32_t) + 1;
}

uint32_t DataValueBlob::GetDataLength() const {
  if (!bKey_ && valType_ == ValueType::NULL_VALUE)
    return 0;
  else
    return soleLength_;
}

uint32_t DataValueBlob::GetMaxLength() const { return maxLength_; }

uint32_t DataValueBlob::GetPersistenceLength() const {
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
void DataValueBlob::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    delete[] soleValue_;

  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = new char[1]{};
  soleLength_ = 1;
}

void DataValueBlob::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    delete[] soleValue_;

  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = new char[4]{ -1, -1, -1, 0 };
  soleLength_ = 4;
}

void DataValueBlob::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    delete[] soleValue_;

  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = new char[1]{ 0 };
  soleLength_ = 1;
}

DataValueBlob::operator const char* () const {
  switch (valType_) {
  case ValueType::NULL_VALUE:
    return nullptr;
  case ValueType::SOLE_VALUE:
    return soleValue_;
  case ValueType::BYTES_VALUE:
    return (char*)byArray_;
  }

  return nullptr;
}

DataValueBlob& DataValueBlob::operator=(const char* val) {
  uint32_t len = *(uint32_t*)val;
  if (len >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
      { to_string(maxLength_), to_string(soleLength_) });
  if (valType_ == ValueType::SOLE_VALUE)
    delete[] soleValue_;

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = new char[soleLength_];
  memcpy(soleValue_, val, soleLength_);
  return *this;
}

void DataValueBlob::Put(uint32_t len, char* val) {
  if (len >= maxLength_)
    throw utils::ErrorMsg(DT_INPUT_OVER_LENGTH,
      { to_string(maxLength_), to_string(soleLength_) });
  if (valType_ == ValueType::SOLE_VALUE)
    delete[] soleValue_;

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = val;
}

DataValueBlob& DataValueBlob::operator=(const DataValueBlob& src) {
  if (valType_ == ValueType::SOLE_VALUE)
    delete[] soleValue_;

  dataType_ = src.dataType_;
  valType_ = src.valType_;
  bKey_ = src.bKey_;
  maxLength_ = src.maxLength_;
  soleLength_ = src.soleLength_;

  switch (valType_) {
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

bool DataValueBlob::operator==(const DataValueBlob& dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return false;
  }

  uint32_t len = GetDataLength();
  if (len != dv.GetDataLength())
    return false;

  const char* v1 =
    (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (char*)byArray_);
  const char* v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_
    : (char*)dv.byArray_);
  return utils::BytesCompare((Byte*)v1, len, (Byte*)v2, len) == 0;
}

std::ostream& operator<<(std::ostream& os, const DataValueBlob& dv) {
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
