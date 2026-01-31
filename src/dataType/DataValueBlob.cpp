#include "DataValueBlob.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <cstring>
#include <stdexcept>

namespace storage {
bool DataValueBlob::SetValue(const char *val, uint32_t len) {
  if (len > _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return false;
  }

  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType = ValueType::SOLE_VALUE;
  _soleLength = len;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, val, len);
  return true;
}

bool DataValueBlob::PutValue(std::any val) {
  const char *buf;
  size_t len = 0;
  if (val.type() == typeid(string)) {
    buf = any_cast<string>(&val)->c_str();
    len = any_cast<string>(val).size();
  } else if (val.type() == typeid(MString)) {
    buf = any_cast<MString>(&val)->c_str();
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
  else if (val.type() == typeid(double))
    buf = toChars(any_cast<double>(val));
  else if (val.type() == typeid(float))
    buf = toChars(any_cast<float>(val));
  else {
    abort();
  }

  if (len == 0)
    len = strlen(buf);

  if (len > _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return false;
  }

  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }
  _valType = ValueType::SOLE_VALUE;
  _soleLength = (uint32_t)len;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, buf, _soleLength);
  return true;
}

bool DataValueBlob::Copy(IDataValue &dv, bool bMove) {
  if (dv.GetRef() > 1) {
    bMove = false;
  };
  if (dataType_ != dv.GetDataType()) {
    _threadErrorMsg.reset(
        new ErrorMsg(DT_UNSUPPORT_CONVERT, {StrOfDataType(dv.GetDataType()),
                                            StrOfDataType(dataType_)}));
    return false;
  }

  if (dv.GetDataLength() > _maxLength) {
    _threadErrorMsg.reset(
        new ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {ToMString(_maxLength), ToMString(dv.GetDataLength())}));
    return false;
  }

  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  if (bMove) {
    _bysValue = ((DataValueBlob &)dv)._bysValue;
    _valType = dv.GetValueType();
    _soleLength = dv.GetDataLength();
    ((DataValueBlob &)dv)._bysValue = nullptr;
    ((DataValueBlob &)dv)._valType = ValueType::NULL_VALUE;
  } else if (dv.GetValueType() == ValueType::BYTES_VALUE) {
    _bysValue = ((DataValueBlob &)dv)._bysValue;
    _valType = ValueType::BYTES_VALUE;
    _soleLength = dv.GetDataLength();
  } else if (dv.GetValueType() != ValueType::NULL_VALUE) {
    _soleLength = dv.GetDataLength();
    _bysValue = CachePool::Apply(_soleLength);
    _valType = ValueType::SOLE_VALUE;
    BytesCopy(_bysValue, ((DataValueBlob &)dv)._bysValue, _soleLength);
  } else {
    _valType = ValueType::NULL_VALUE;
    _soleLength = 0;
    _bysValue = nullptr;
  }

  return true;
}

uint32_t DataValueBlob::WriteData(Byte *buf, SavePosition dtPos) const {
  assert(dtPos == SavePosition::VALUE);
  if (_valType == ValueType::NULL_VALUE) {
    return 0;
  } else {
    BytesCopy(buf, _bysValue, _soleLength);
    return _soleLength;
  }
}

uint32_t DataValueBlob::ReadData(const Byte *buf, uint32_t len,
                                 SavePosition dtPos, bool bSole) {
  assert(dtPos == SavePosition::VALUE);
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  if (len == 0) {
    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
    return 0;
  }

  if (len > _maxLength)
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
  _soleLength = len;
  if (bSole) {
    _bysValue = CachePool::Apply(_soleLength);
    BytesCopy(_bysValue, buf, _soleLength);
    _valType = ValueType::SOLE_VALUE;
  } else {
    _valType = ValueType::BYTES_VALUE;
    _bysValue = const_cast<Byte *>(buf);
  }

  return _soleLength;
}

uint32_t DataValueBlob::WriteData(Byte *buf) const {
  if (_valType == ValueType::NULL_VALUE) {
    buf[0] = ((Byte)DataType::BLOB & DATE_TYPE);
    return 1;
  } else {
    buf[0] = (VALUE_TYPE | ((Byte)DataType::BLOB & DATE_TYPE));
    BytesCopy(buf + 1, (Byte *)&_soleLength, sizeof(uint32_t));
    BytesCopy(buf + 1 + sizeof(uint32_t), _bysValue, _soleLength);
    return _soleLength + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueBlob::ReadData(const Byte *buf) {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType =
      ((buf[0] & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (_valType == ValueType::NULL_VALUE) {
    return 1;
  }

  _valType = ValueType::SOLE_VALUE;
  BytesCopy((Byte *)&_soleLength, buf + 1, sizeof(uint32_t));
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, buf + 1 + sizeof(uint32_t), _soleLength);
  return _soleLength + sizeof(uint32_t) + 1;
}

void DataValueBlob::SetMinValue() {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType = ValueType::SOLE_VALUE;
  _soleLength = 1;
  _bysValue = CachePool::Apply(_soleLength);
  _bysValue[0] = 0;
}

void DataValueBlob::SetMaxValue() {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType = ValueType::SOLE_VALUE;
  _soleLength = _maxLength;
  _bysValue = CachePool::Apply(_soleLength);
  memset(_bysValue, UINT8_MAX, _soleLength);
}

void DataValueBlob::SetDefaultValue() {
  if (_valType == ValueType::SOLE_VALUE)
    CachePool::Release(_bysValue, _soleLength);

  _valType = ValueType::SOLE_VALUE;
  _soleLength = 1;
  _bysValue = CachePool::Apply(_soleLength);
  _bysValue[0] = 0;
}

DataValueBlob::operator const char *() const {
  switch (_valType) {
  case ValueType::NULL_VALUE:
    return nullptr;
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    return (char *)_bysValue;
  }

  return nullptr;
}

bool DataValueBlob::operator==(const DataValueBlob &dv) const {
  if (_valType == ValueType::NULL_VALUE) {
    return dv._valType == ValueType::NULL_VALUE;
  }
  if (dv._valType == ValueType::NULL_VALUE) {
    return false;
  }

  uint32_t len = GetDataLength();
  if (len != dv.GetDataLength())
    return false;

  return BytesCompare(_bysValue, len, dv._bysValue, len) == 0;
}

void DataValueBlob::ToString(StrBuff &sb) const {
  if (_valType == ValueType::NULL_VALUE) {
    return;
  }
  if (_soleLength * 2 + 3 > sb.GetFreeLen()) {
    sb.Resize(sb.GetStrLen() + _soleLength * 2 + 3);
  }

  Byte *src = _bysValue;
  char *dest = sb.GetFreeBuff();
  std::strcpy(dest, "0x");
  dest += 2;
  for (uint32_t i = 0; i < _soleLength; i++) {
    std::sprintf(dest, "%02X", *src);
    src++;
    dest += 2;
  }

  sb.SetStrLen(sb.GetStrLen() + _soleLength * 2 + 2);
}

std::ostream &operator<<(std::ostream &os, const DataValueBlob &dv) {
  switch (dv._valType) {
  case ValueType::NULL_VALUE:
    os << "nullptr";
    break;
  case ValueType::SOLE_VALUE:
    os << "size=" << dv._soleLength << "\tValType:" << ValueType::SOLE_VALUE;
    break;
  case ValueType::BYTES_VALUE:
    os << "size=" << dv._soleLength << "\tValType:" << ValueType::BYTES_VALUE;
    break;
  }

  return os;
}
} // namespace storage
