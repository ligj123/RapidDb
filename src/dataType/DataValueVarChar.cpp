#include "DataValueVarChar.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include <cstring>
#include <stdexcept>

namespace storage {
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
    throw ErrorMsg(DT_UNSUPPORT_CONVERT,
                   {val.type().name(), "DataValueVarChar"});

  soleLength_ = (uint32_t)str.size() + 1;
  if (soleLength_ >= maxLength_)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});

  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, str.c_str(), soleLength_);
}

void DataValueVarChar::Copy(const IDataValue &dv, bool bMove) {
  if (dv.IsNull()) {
    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, soleLength_);
    }
    valType_ = ValueType::NULL_VALUE;
  }

  if (dataType_ != dv.GetDataType()) {
    StrBuff sb(0);
    dv.ToString(sb);
    if (maxLength_ < sb.GetStrLen() + 1) {
      throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {to_string(maxLength_), to_string(dv.GetDataLength())});
    }

    if (valType_ == ValueType::SOLE_VALUE) {
      CachePool::Release(bysValue_, soleLength_);
    }

    soleLength_ = sb.GetStrLen() + 1;
    bysValue_ = CachePool::Apply(soleLength_);
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, sb.GetBuff(), soleLength_);
    return;
  }

  if (dv.GetDataLength() > maxLength_) {
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(dv.GetDataLength())});
  }

  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (dv.GetValueType() == ValueType::BYTES_VALUE) {
    bysValue_ = ((DataValueVarChar &)dv).bysValue_;
    valType_ = ValueType::BYTES_VALUE;
    soleLength_ = dv.GetDataLength();
  } else if (bMove) {
    bysValue_ = ((DataValueVarChar &)dv).bysValue_;
    valType_ = ValueType::SOLE_VALUE;
    soleLength_ = dv.GetDataLength();
    ((DataValueVarChar &)dv).bysValue_ = nullptr;
    ((DataValueVarChar &)dv).valType_ = ValueType::NULL_VALUE;
  } else if (dv.GetValueType() != ValueType::NULL_VALUE) {
    soleLength_ = dv.GetDataLength();
    bysValue_ = CachePool::Apply(soleLength_);
    valType_ = ValueType::SOLE_VALUE;
    BytesCopy(bysValue_, ((DataValueVarChar &)dv).bysValue_, soleLength_);
  }
}

uint32_t DataValueVarChar::WriteData(Byte *buf, bool key) const {
  if (bKey_) {
    // Write default value if is null for key
    if (valType_ == ValueType::NULL_VALUE) {
      *buf = 0;
      return 1;
    }
    BytesCopy(buf, bysValue_, soleLength_);
    return soleLength_;
  } else {
    if (valType_ == ValueType::NULL_VALUE) {
      return 0;
    } else {
      BytesCopy(buf, bysValue_, soleLength_);
      return soleLength_;
    }
  }
}

uint32_t DataValueVarChar::WriteData(fstream &fs) const {
  if (valType_ == ValueType::NULL_VALUE) {
    fs.put((Byte)DataType::VARCHAR & DATE_TYPE);
    return 1;
  } else {
    fs.put((char)(VALUE_TYPE | ((Byte)DataType::VARCHAR & DATE_TYPE)));
    fs.write((char *)&soleLength_, sizeof(uint32_t));
    fs.write((char *)bysValue_, soleLength_);
    return soleLength_ + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueVarChar::ReadData(fstream &fs) {
  char c;
  fs.get(c);

  valType_ = (c & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (valType_ == ValueType::NULL_VALUE) {
    valType_ = ValueType::NULL_VALUE;
    return 1;
  }

  valType_ = ValueType::SOLE_VALUE;
  fs.read((char *)&soleLength_, sizeof(uint32_t));
  bysValue_ = CachePool::Apply(soleLength_);
  fs.read((char *)bysValue_, soleLength_);
  return soleLength_ + sizeof(uint32_t) + 1;
}

uint32_t DataValueVarChar::ReadData(Byte *buf, uint32_t len, bool bSole) {
  if (valType_ == ValueType::SOLE_VALUE) {
    CachePool::Release(bysValue_, soleLength_);
  }

  if (bKey_) {
    assert(len > 0);
    soleLength_ = len;
    if (bSole) {
      valType_ = ValueType::SOLE_VALUE;
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, buf, len);
    } else {
      valType_ = ValueType::BYTES_VALUE;
      bysValue_ = buf;
    }

    return len;
  } else {
    if (len == 0) {
      valType_ = ValueType::NULL_VALUE;
      return 0;
    }

    soleLength_ = len;
    if (bSole) {
      bysValue_ = CachePool::Apply(soleLength_);
      BytesCopy(bysValue_, buf, soleLength_);
      valType_ = ValueType::SOLE_VALUE;
    } else {
      bysValue_ = buf;
      valType_ = ValueType::BYTES_VALUE;
    }
    return soleLength_;
  }
}

void DataValueVarChar::SetMinValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = 0;
}

void DataValueVarChar::SetMaxValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 4;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = bysValue_[1] = bysValue_[2] = -1;
  bysValue_[3] = 0;
}

void DataValueVarChar::SetDefaultValue() {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  valType_ = ValueType::SOLE_VALUE;
  soleLength_ = 1;
  bysValue_ = CachePool::Apply(soleLength_);
  bysValue_[0] = 0;
}

DataValueVarChar &DataValueVarChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val) + 1;
  if (len >= maxLength_ - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val, soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const string val) {
  uint32_t len = (uint32_t)val.size() + 1;
  if (len >= maxLength_ - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {to_string(maxLength_), to_string(soleLength_)});
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release(bysValue_, soleLength_);

  soleLength_ = len;
  valType_ = ValueType::SOLE_VALUE;
  bysValue_ = CachePool::Apply(soleLength_);
  BytesCopy(bysValue_, val.c_str(), soleLength_);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const DataValueVarChar &src) {
  if (valType_ == ValueType::SOLE_VALUE)
    CachePool::Release((Byte *)bysValue_, soleLength_);

  dataType_ = src.dataType_;
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
    bysValue_ = nullptr;
    break;
  }

  return *this;
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

} // namespace storage
