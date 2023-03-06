#include "DataValueBlob.h"
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <cstring>
#include <stdexcept>

namespace storage {
bool DataValueBlob::SetValue(const char *val, int len) {
  if (len + 1 >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return false;
  }

  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = len + 1;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val, len);
  bysValue_[len - 1] = 0;
  return true;
}

bool DataValueBlob::PutValue(std::any val) {
  const char *buf;
  size_t len = 0;
  if (val.type() == typeid(string)) {
    buf = any_cast<string>(val).c_str();
    len = any_cast<string>(val).size();
  } else if (val.type() == typeid(MString)) {
    buf = any_cast<MString>(val).c_str();
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
  else {
    _threadErrorMsg.reset(new ErrorMsg(DT_UNSUPPORT_CONVERT,
                                       {val.type().name(), "DataValueBlob"}));
    return false;
  }

  if (len == 0)
    len = strlen(buf);

  if (len + 1 >= maxLength_) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
    return false;
  }

  valType_ = ValueType::BYTES_VALUE;
  soleLength_ = len + 1;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, buf, soleLength_);
}

bool DataValueBlob::Copy(const IDataValue &dv, bool bMove) {
  if (dataType_ != dv.GetDataType()) {
    _threadErrorMsg.reset(
        new ErrorMsg(DT_UNSUPPORT_CONVERT, {StrOfDataType(dv.GetDataType()),
                                            StrOfDataType(dataType_)}));
    return false;
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
    soleLength_ = dv.GetDataLength();
    bysValue_ = CachePool::Apply(soleLength_);
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, ((DataValueBlob &)dv).bysValue_, soleLength_);
  } else {
    valType_ = ValueType::NULL_VALUE;
    soleLength_ = 0;
    bysValue_ = nullptr;
  }

  return true;
}

uint32_t DataValueBlob::WriteData(Byte *buf, SavePosition dtPos) const {
  assert(dtPos == SavePosition::VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    return 0;
  } else {
    BytesCopy(buf, bysValue_, soleLength_);
    return soleLength_;
  }
}

uint32_t DataValueBlob::ReadData(Byte *buf, uint32_t len, SavePosition dtPos,
                                 bool bSole) {
  assert(dtPos == SavePosition::VALUE);
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (len == 0) {
    valType_ = ValueType::NULL_VALUE;
    bysValue_ = nullptr;
    return 0;
  }

  if (len > maxLength_)
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {to_string(maxLength_), to_string(len)}));
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

uint32_t DataValueBlob::WriteData(Byte *buf) const {
  if (valType_ == ValueType::NULL_VALUE) {
    buf[0] = ((Byte)DataType::BLOB & DATE_TYPE);
    return 1;
  } else {
    buf[0] = (VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE));
    BytesCopy(buf + 1, (Byte *)&soleLength_, sizeof(uint32_t));
    BytesCopy(buf + 1 + sizeof(uint32_t), bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueBlob::ReadData(Byte *buf) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  valType_ =
      ((buf[0] & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    return 1;
  }

  valType_ = ValueType::SOLE_VALUE;
  BytesCopy((Byte *)&soleLength_, buf + 1, sizeof(uint32_t));
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, buf + 1 + sizeof(uint32_t), soleLength_);
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

DataValueBlob &DataValueBlob::operator=(const DataValueBlob &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

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
