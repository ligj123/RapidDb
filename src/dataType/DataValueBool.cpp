#include "DataValueBool.h"
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace storage {
DataValueBool::DataValueBool(bool bKey)
    : IDataValue(DataType::BOOL, ValueType::NULL_VALUE, bKey) {}

DataValueBool::DataValueBool(bool val, bool bKey)
    : IDataValue(DataType::BOOL, ValueType::SOLE_VALUE, bKey), soleValue_(val) {
}

DataValueBool::DataValueBool(Byte *byArray, bool bKey)
    : IDataValue(DataType::BOOL, ValueType::BYTES_VALUE, bKey),
      byArray_(byArray) {}

DataValueBool::DataValueBool(std::any val, bool bKey)
    : IDataValue(DataType::BOOL, ValueType::SOLE_VALUE, bKey) {
  if (val.type() == typeid(bool))
    soleValue_ = std::any_cast<bool>(val);
  else if (val.type() == typeid(int64_t))
    soleValue_ = (bool)std::any_cast<int64_t>(val);
  else if (val.type() == typeid(int32_t))
    soleValue_ = (bool)std::any_cast<int32_t>(val);
  else if (val.type() == typeid(int16_t))
    soleValue_ = (bool)std::any_cast<int16_t>(val);
  else if (val.type() == typeid(uint64_t))
    soleValue_ = (bool)std::any_cast<uint64_t>(val);
  else if (val.type() == typeid(uint32_t))
    soleValue_ = (bool)std::any_cast<uint32_t>(val);
  else if (val.type() == typeid(uint16_t))
    soleValue_ = (bool)std::any_cast<uint16_t>(val);
  else if (val.type() == typeid(int8_t))
    soleValue_ = (bool)std::any_cast<int8_t>(val);
  else if (val.type() == typeid(uint8_t))
    soleValue_ = (bool)std::any_cast<uint8_t>(val);
  else if (val.type() == typeid(std::string)) {
    string str = std::any_cast<std::string>(val);
    transform(str.begin(), str.end(), str.begin(),
              [](unsigned char c) -> unsigned char { return std::tolower(c); });
    if (str.find("true"))
      soleValue_ = true;
    else if (str.find("false"))
      soleValue_ = false;
    else
      soleValue_ = (bool)std::stoi(str);
  } else
    throw utils::ErrorMsg(DT_UNSUPPORT_CONVERT,
                          {val.type().name(), "DataValueBool"});
}

DataValueBool::DataValueBool(const DataValueBool &src) : IDataValue(src) {
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

DataValueBool *DataValueBool::CloneDataValue(bool incVal) {
  return new DataValueBool(*this);
}

std::any DataValueBool::GetValue() const {
  switch (valType_) {
  case ValueType::SOLE_VALUE:
    return soleValue_;
  case ValueType::BYTES_VALUE:
    return (bool)(*byArray_);
  case ValueType::NULL_VALUE:
  default:
    return std::any();
  }
}

uint32_t DataValueBool::GetPersistenceLength(bool key) const {
  return key ? 1 : (valType_ == ValueType::NULL_VALUE ? 1 : 2);
}

uint32_t DataValueBool::WriteData(Byte *buf, bool key) {
  if (key) {
    assert(valType_ != ValueType::NULL_VALUE);
    if (valType_ == ValueType::BYTES_VALUE) {
      *buf = *byArray_;
    } else if (valType_ == ValueType::SOLE_VALUE) {
      *buf = soleValue_;
    }

    return 1;
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      *buf = (Byte)DataType::BOOL & DATE_TYPE;
      return 1;
    } else if (valType_ == ValueType::BYTES_VALUE) {
      *buf = VALUE_TYPE | ((Byte)DataType::BOOL & DATE_TYPE);
      buf++;
      *buf = *byArray_;
      return 2;
    } else {
      *buf = VALUE_TYPE | ((Byte)DataType::BOOL & DATE_TYPE);
      buf++;
      *buf = soleValue_;
      return 2;
    }
  }
}

uint32_t DataValueBool::WriteData(fstream &fs) {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::BOOL & DATE_TYPE);
    return 1;
  } else {
    fs.put(VALUE_TYPE | ((Byte)DataType::BOOL & DATE_TYPE));
    if (valType_ == ValueType::SOLE_VALUE) {
      fs.write((char *)&soleValue_, 1);
    } else {
      fs.write((char *)byArray_, 1);
    }
    return 2;
  }
}

uint32_t DataValueBool::ReadData(fstream &fs) {
  Byte by;
  fs.read((char *)&by, 1);
  valType_ =
      ((by & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE)
    return 1;

  fs.read((char *)&soleValue_, 1);
  return 2;
}

uint32_t DataValueBool::ReadData(Byte *buf, uint32_t len, bool bSole) {
  if (bKey_) {
    valType_ = ValueType::SOLE_VALUE;
    soleValue_ = *buf;
    return 1;
  } else {
    valType_ =
        ((*buf & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);

    if (valType_ == ValueType::NULL_VALUE)
      return 1;

    buf++;
    soleValue_ = *buf;
    return 2;
  }
}

void DataValueBool::SetMinValue() {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = false;
}
void DataValueBool::SetMaxValue() {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = true;
}
void DataValueBool::SetDefaultValue() {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = false;
}

DataValueBool::operator bool() const {
  switch (valType_) {
  case ValueType::NULL_VALUE:
    return false;
  case ValueType::SOLE_VALUE:
    return soleValue_;
  case ValueType::BYTES_VALUE:
    return (bool)(*byArray_);
  }

  return 0;
}

DataValueBool &DataValueBool::operator=(bool val) {
  valType_ = ValueType::SOLE_VALUE;
  soleValue_ = val;
  return *this;
}

DataValueBool &DataValueBool::operator=(const DataValueBool &src) {
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

bool DataValueBool::operator>(const DataValueBool &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return false;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return true;
  }

  bool v1 =
      (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (bool)(*byArray_));
  bool v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_
                                                  : (bool)(*dv.byArray_));
  return v1 > v2;
}

bool DataValueBool::operator<(const DataValueBool &dv) const {
  return !(*this >= dv);
}

bool DataValueBool::operator>=(const DataValueBool &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return true;
  }

  bool v1 =
      (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (bool)(*byArray_));
  bool v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_
                                                  : (bool)(*dv.byArray_));
  return v1 >= v2;
}

bool DataValueBool::operator<=(const DataValueBool &dv) const {
  return !(*this > dv);
}

bool DataValueBool::operator==(const DataValueBool &dv) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return dv.valType_ == ValueType::NULL_VALUE;
  }
  if (dv.valType_ == ValueType::NULL_VALUE) {
    return false;
  }

  uint8_t v1 =
      (valType_ == ValueType::SOLE_VALUE ? soleValue_ : (bool)(*byArray_));
  uint8_t v2 = (dv.valType_ == ValueType::SOLE_VALUE ? dv.soleValue_
                                                     : (bool)(*byArray_));
  return v1 == v2;
}

bool DataValueBool::operator!=(const DataValueBool &dv) const {
  return !(*this == dv);
}

std::ostream &operator<<(std::ostream &os, const DataValueBool &dv) {
  switch (dv.valType_) {
  case ValueType::NULL_VALUE:
    os << "nullptr";
    break;
  case ValueType::SOLE_VALUE:
    os << (dv.soleValue_ ? "true" : " false");
    break;
  case ValueType::BYTES_VALUE:
    os << (*dv.byArray_ ? "true" : " false");
    break;
  }

  return os;
}

void DataValueBool::ToString(StrBuff &sb) const {
  if (valType_ == ValueType::NULL_VALUE) {
    return;
  }
  if (6 > sb.GetFreeLen()) {
    sb.Resize(sb.GetStrLen() + 6);
  }

  bool b = (valType_ == ValueType::SOLE_VALUE ? soleValue_ : *byArray_);
  char *dest = sb.GetFreeBuff();
  strcpy(dest, b ? "true" : "false");
  sb.SetStrLen(sb.GetStrLen() + b ? 4 : 5);
}
} // namespace storage
