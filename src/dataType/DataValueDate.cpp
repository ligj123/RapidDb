#include "DataValueDate.h"
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include <cstring>
#include <limits.h>
#include <stdexcept>
#include <string>

namespace storage {
DataValueDate::DataValueDate(bool bKey)
    : IDataValue(DataType::DATETIME, ValueType::NULL_VALUE, bKey) {}

DataValueDate::DataValueDate(uint64_t msVal, bool bKey)
    : IDataValue(DataType::DATETIME, ValueType::SOLE_VALUE, bKey),
      soleValue_(msVal) {}

DataValueDate::DataValueDate(Byte *byArray, bool bKey)
    : IDataValue(DataType::DATETIME, ValueType::BYTES_VALUE, bKey),
      byArray_(byArray) {}

DataValueDate::DataValueDate(std::any val, bool bKey)
    : IDataValue(DataType::DATETIME, ValueType::SOLE_VALUE, bKey) {
  if (val.type() == typeid(uint64_t))
    soleValue_ = (uint64_t)std::any_cast<uint64_t>(val);
  else if (val.type() == typeid(int64_t))
    soleValue_ = (uint64_t)std::any_cast<int64_t>(val);
  else
    throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT,
                          {val.type().name(), "DataValueDate"});
}

DataValueDate::DataValueDate(const DataValueDate &src) : IDataValue(src) {
  switch (src.valType_) {
  case ValueType::SOLE_VALUE:
    soleValue_ = src.soleValue_;
    break;
  case ValueType::BYTES_VALUE:
    byArray_ = src.byArray_;
    break;
  case ValueType::NULL_VALUE:
    break;
  }
}

DataValueDate *DataValueDate::CloneDataValue(bool incVal) {
  return new DataValueDate(*this);
}

std::any DataValueDate::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
    return soleValue_;
  case ValueType::BYTES_VALUE:
    return utils::UInt64FromBytes(byArray_, bKey_);
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueDate::WriteData(Byte *buf) { return WriteData(buf, bKey_); }

uint32_t DataValueDate::GetPersistenceLength(bool key) const {
  return key ? sizeof(uint64_t)
             : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(uint64_t));
}

uint32_t DataValueDate::WriteData(Byte *buf, bool key) {
  if (key) {
    assert(valType_ != ValueType::NULL_VALUE);
    if (valType_ == ValueType::BYTES_VALUE) {
      std::memcpy(buf, byArray_, sizeof(uint64_t));
    } else if (valType_ == ValueType::SOLE_VALUE) {
      utils::UInt64ToBytes(soleValue_, buf, bKey_);
    }

    return sizeof(uint64_t);
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      *buf = (Byte)DataType::DATETIME & DATE_TYPE;
      return 1;
    } else if (valType_ == ValueType::BYTES_VALUE) {
      *buf = VALUE_TYPE | ((Byte)DataType::DATETIME & DATE_TYPE);
      buf++;
      std::memcpy(buf, byArray_, sizeof(uint64_t));
      return sizeof(uint64_t) + 1;
    } else {
      *buf = VALUE_TYPE | ((Byte)DataType::DATETIME & DATE_TYPE);
      buf++;
      utils::UInt64ToBytes(soleValue_, buf, bKey_);
      return sizeof(uint64_t) + 1;
    }
  }
}

uint32_t DataValueDate::WriteData(fstream &fs) {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::DATETIME & DATE_TYPE);
    return 1;
  } else {
    fs.put(VALUE_TYPE | ((Byte)DataType::DATETIME & DATE_TYPE));
    if (valType_ == ValueType::SOLE_VALUE) {
      fs.write((char *)&soleValue_, sizeof(uint64_t));
    } else {
      fs.write((char *)byArray_, sizeof(uint64_t));
    }
    return sizeof(uint64_t) + 1;
  }
}

uint32_t DataValueDate::ReadData(fstream &fs) {
  Byte by;
  fs.read((char *)&by, 1);
  valType_ =
      ((by & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE)
    return 1;

  fs.read((char *)&soleValue_, sizeof(uint64_t));
  return sizeof(uint64_t) + 1;
}

uint32_t DataValueDate::ReadData(Byte *buf, uint32_t len, bool bSole) {
  if (bKey_) {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = utils::UInt64FromBytes(buf, bKey_);
    return sizeof(uint64_t);
  } else {
    valType_ =
        (*buf & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
    buf++;

    if (valType_ == ValueType::NULL_VALUE)
      return 1;

    soleValue_ = utils::UInt64FromBytes(buf, bKey_);
    return sizeof(uint64_t) + 1;
  }
}

uint32_t DataValueDate::GetDataLength() const {
  return bKey_ ? sizeof(uint64_t)
               : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(uint64_t));
}

uint32_t DataValueDate::GetMaxLength() const { return sizeof(uint64_t); }

uint32_t DataValueDate::GetPersistenceLength() const {
  return bKey_ ? sizeof(uint64_t)
               : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(uint64_t));
}

void DataValueDate::SetMinValue() {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = 0;
}
void DataValueDate::SetMaxValue() {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = ULLONG_MAX;
}
void DataValueDate::SetDefaultValue() {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = 0;
}

DataValueDate::operator uint64_t() const {
  switch (valType_) {
  case ValueType::NULL_VALUE:
    return 0;
  case ValueType::SOLE_VALUE:
    return soleValue_;
  case ValueType::BYTES_VALUE:
    return utils::UInt64FromBytes(byArray_, bKey_);
    ;
  }

  return 0;
}

DataValueDate &DataValueDate::operator=(uint64_t val) {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = val;
  return *this;
}

DataValueDate &DataValueDate::operator=(const DataValueDate &src) {
  dataType_ = src.dataType_;
  valType_ = src.valType_;
  bKey_ = src.bKey_;
  switch (src.valType_) {
  case ValueType::SOLE_VALUE:
    soleValue_ = src.soleValue_;
    break;
  case ValueType::BYTES_VALUE:
    byArray_ = src.byArray_;
    break;
  case ValueType::NULL_VALUE:
    break;
  }

  return *this;
}

bool DataValueDate::operator>(const DataValueDate &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return false;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return true;
  }

  uint64_t v1 = (valType_ == ValueType::SOLE_VALUE
                     ? soleValue_
                     : utils::UInt64FromBytes(byArray_, bKey_));
  uint64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE
                     ? dv.soleValue_
                     : utils::UInt64FromBytes(dv.byArray_, bKey_));
  return v1 > v2;
}

bool DataValueDate::operator<(const DataValueDate &dv) const {
  return !(*this >= dv);
}

bool DataValueDate::operator>=(const DataValueDate &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return true;
  }

  uint64_t v1 = (valType_ == ValueType::SOLE_VALUE
                     ? soleValue_
                     : utils::UInt64FromBytes(byArray_, bKey_));
  uint64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE
                     ? dv.soleValue_
                     : utils::UInt64FromBytes(dv.byArray_, bKey_));
  return v1 >= v2;
}

bool DataValueDate::operator<=(const DataValueDate &dv) const {
  return !(*this > dv);
}

bool DataValueDate::operator==(const DataValueDate &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return false;
  }

  uint64_t v1 = (valType_ == ValueType::SOLE_VALUE
                     ? soleValue_
                     : utils::UInt64FromBytes(byArray_, bKey_));
  uint64_t v2 = (dv.valType_ == ValueType::SOLE_VALUE
                     ? dv.soleValue_
                     : utils::UInt64FromBytes(dv.byArray_, bKey_));
  return v1 == v2;
}

bool DataValueDate::operator!=(const DataValueDate &dv) const {
  return !(*this == dv);
}

std::ostream &operator<<(std::ostream &os, const DataValueDate &dv) {
  switch (dv.valType_) {
  case ValueType::NULL_VALUE:
    os << "nullptr";
    break;
  case ValueType::SOLE_VALUE:
    os << dv.soleValue_;
    break;
  case ValueType::BYTES_VALUE:
    os << utils::UInt64FromBytes(dv.byArray_, dv.bKey_);
    break;
  }

  return os;
}

void DataValueDate::ToString(StrBuff &sb) {
  if (valType_ == ValueType::NULL_VALUE) {
    return;
  }
  if (22 > sb.GetFreeLen()) {
    sb.Resize(sb.GetStrLen() + 22);
  }

  uint64_t t = (valType_ == ValueType::SOLE_VALUE
                    ? soleValue_
                    : utils::UInt64FromBytes(byArray_, bKey_));
  char *dest = sb.GetFreeBuff();
  int n = sprintf(dest, "%llu", t);
  sb.SetStrLen(sb.GetStrLen() + n);
}
} // namespace storage
