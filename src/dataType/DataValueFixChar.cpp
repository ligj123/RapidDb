#include "DataValueFixChar.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace storage {
bool DataValueFixChar::SetValue(const char *val, uint32_t len) {
  if (len >= _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return false;
  }

  if (_valType != ValueType::SOLE_VALUE) {
    _bysValue = CachePool::Apply(_maxLength);
  }

  _valType = ValueType::SOLE_VALUE;
  BytesCopy(_bysValue, val, len);
  memset(_bysValue + len, ' ', _maxLength - len);
  return true;
}

bool DataValueFixChar::PutValue(std::any val) {
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

  if (_valType != ValueType::SOLE_VALUE)
    _bysValue = CachePool::Apply(_maxLength);

  _valType = ValueType::SOLE_VALUE;
  BytesCopy(_bysValue, buf, len);
  memset(_bysValue + len, ' ', _maxLength - len);
  return true;
}

bool DataValueFixChar::Copy(IDataValue &dv, bool bMove) {
  if (dv.GetRef() > 1) {
    bMove = false;
  };
  if (dv.IsNull()) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _maxLength);
    }
    _bysValue = nullptr;
    _valType = ValueType::NULL_VALUE;
    return true;
  }

  if (!dv.IsStringType()) {
    StrBuff sb(0);
    dv.ToString(sb);
    if (_maxLength < sb.GetStrLen()) {
      _threadErrorMsg.reset(
          new ErrorMsg(DT_INPUT_OVER_LENGTH,
                       {ToMString(_maxLength), ToMString(sb.GetStrLen())}));
      return false;
    }

    if (_valType != ValueType::SOLE_VALUE) {
      _bysValue = CachePool::Apply(_maxLength);
    }

    int len = sb.GetStrLen();
    _valType = ValueType::SOLE_VALUE;
    BytesCopy(_bysValue, sb.GetBuff(), len);
    memset(_bysValue + len, ' ', _maxLength - len);
    return true;
  }

  if (dv.GetDataLength() > _maxLength) {
    _threadErrorMsg.reset(
        new ErrorMsg(DT_INPUT_OVER_LENGTH,
                     {ToMString(_maxLength), ToMString(dv.GetDataLength())}));
    return false;
  }

  if (bMove && dv.GetDataType() == DataType::FIXCHAR &&
      _maxLength == dv.GetMaxLength()) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _maxLength);
    }
    _bysValue = ((DataValueFixChar &)dv)._bysValue;
    _valType = dv.GetValueType();
    ((DataValueFixChar &)dv)._bysValue = nullptr;
    ((DataValueFixChar &)dv)._valType = ValueType::NULL_VALUE;
  } else if (dv.GetValueType() == ValueType::BYTES_VALUE &&
             _maxLength == dv.GetMaxLength()) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _maxLength);
    }

    _bysValue = ((DataValueFixChar &)dv)._bysValue;
    _valType = ValueType::BYTES_VALUE;
  } else {
    if (_valType != ValueType::SOLE_VALUE) {
      _bysValue = CachePool::Apply(_maxLength);
    }
    _valType = ValueType::SOLE_VALUE;
    BytesCopy(_bysValue, ((DataValueFixChar &)dv)._bysValue,
              dv.GetMaxLength() - 1);
    if (_maxLength > dv.GetMaxLength()) {
      memset(_bysValue + dv.GetMaxLength(), ' ',
             _maxLength - dv.GetMaxLength());
    }
  }
  return true;
}

uint32_t DataValueFixChar::WriteData(Byte *buf, SavePosition svPos) const {
  if (svPos == SavePosition::KEY) {
    if (_valType == ValueType::NULL_VALUE) {
      memset(buf, ' ', _maxLength);
    } else {
      BytesCopy(buf, _bysValue, _maxLength);
    }
    return _maxLength;
  } else {
    if (_valType == ValueType::NULL_VALUE) {
      return 0;
    } else {
      BytesCopy(buf, _bysValue, _maxLength);
      return _maxLength;
    }
  }
}

uint32_t DataValueFixChar::ReadData(const Byte *buf, uint32_t len,
                                    SavePosition svPos, bool bSole) {
  assert(len == 0 || len == _maxLength);
  if (svPos == SavePosition::KEY) {
    assert(len > 0);
    if (bSole) {
      if (_valType != ValueType::SOLE_VALUE) {
        _bysValue = CachePool::Apply(_maxLength);
      }
      _valType = ValueType::SOLE_VALUE;
      BytesCopy(_bysValue, buf, _maxLength);
    } else {
      if (_valType == ValueType::SOLE_VALUE) {
        CachePool::Release(_bysValue, _maxLength);
      }
      _valType = ValueType::BYTES_VALUE;
      _bysValue = const_cast<Byte *>(buf);
    }
    return _maxLength;
  } else {
    if (len == 0) {
      if (_valType == ValueType::SOLE_VALUE) {
        CachePool::Release(_bysValue, _maxLength);
      }
      _valType = ValueType::NULL_VALUE;
      return 0;
    }

    if (bSole) {
      if (_valType != ValueType::SOLE_VALUE)
        _bysValue = CachePool::Apply(_maxLength);
      _valType = ValueType::SOLE_VALUE;
      BytesCopy(_bysValue, buf, _maxLength);
    } else {
      if (_valType == ValueType::SOLE_VALUE)
        CachePool::Release(_bysValue, _maxLength);
      _valType = ValueType::BYTES_VALUE;
      _bysValue = const_cast<Byte *>(buf);
    }

    return _maxLength;
  }
}

uint32_t DataValueFixChar::WriteData(Byte *buf) const {
  if (_valType == ValueType::NULL_VALUE) {
    buf[0] = (Byte)DataType::FIXCHAR & DATE_TYPE;
    return 1;
  } else {
    buf[0] = (Byte)(VALUE_TYPE | ((Byte)DataType::FIXCHAR & DATE_TYPE));
    BytesCopy(buf + 1, (Byte *)&_maxLength, sizeof(uint32_t));
    BytesCopy(buf + 1 + sizeof(uint32_t), _bysValue, _maxLength);
    return _maxLength + 1;
  }
}

uint32_t DataValueFixChar::ReadData(const Byte *buf) {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _maxLength);
  }

  _valType =
      (buf[0] & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (_valType == ValueType::NULL_VALUE) {
    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
    return 1;
  }

  _valType = ValueType::SOLE_VALUE;
  BytesCopy((Byte *)&_maxLength, buf + 1, sizeof(uint32_t));
  _bysValue = CachePool::Apply(_maxLength);
  BytesCopy(_bysValue, buf + 1 + sizeof(uint32_t), _maxLength);
  return _maxLength + 1;
}

void DataValueFixChar::SetMinValue() {
  if (_valType != ValueType::SOLE_VALUE) {
    _bysValue = CachePool::Apply(_maxLength);
  }
  _valType = ValueType::SOLE_VALUE;
  memset(_bysValue, 0, _maxLength);
}

void DataValueFixChar::SetMaxValue() {
  if (_valType != ValueType::SOLE_VALUE) {
    _bysValue = CachePool::Apply(_maxLength);
  }

  _valType = ValueType::SOLE_VALUE;
  memset(_bysValue, 0xFF, _maxLength);
}

void DataValueFixChar::SetDefaultValue() {
  if (_valType != ValueType::SOLE_VALUE) {
    _bysValue = CachePool::Apply(_maxLength);
  }

  _valType = ValueType::SOLE_VALUE;
  memset(_bysValue, ' ', _maxLength);
}

DataValueFixChar *DataValueFixChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val);
  if (len >= _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return nullptr;
  }
  if (_valType != ValueType::SOLE_VALUE)
    _bysValue = CachePool::Apply(_maxLength);

  _valType = ValueType::SOLE_VALUE;
  BytesCopy(_bysValue, val, len);
  memset(_bysValue + len, ' ', _maxLength - len);
  return this;
}

DataValueFixChar *DataValueFixChar::operator=(const MString val) {
  uint32_t len = (uint32_t)val.size();
  if (len >= _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return nullptr;
  }
  if (_valType != ValueType::SOLE_VALUE)
    _bysValue = CachePool::Apply(_maxLength);

  _valType = ValueType::SOLE_VALUE;
  BytesCopy(_bysValue, val.c_str(), len);
  memset(_bysValue + len, ' ', _maxLength - len);
  return this;
}

DataValueFixChar *DataValueFixChar::operator=(const string val) {
  uint32_t len = (uint32_t)val.size();
  if (len >= _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return nullptr;
  }
  if (_valType != ValueType::SOLE_VALUE)
    _bysValue = CachePool::Apply(_maxLength);

  _valType = ValueType::SOLE_VALUE;
  BytesCopy(_bysValue, val.c_str(), len);
  memset(_bysValue + len, ' ', _maxLength - len);
  return this;
}

std::ostream &operator<<(std::ostream &os, const DataValueFixChar &dv) {
  switch (dv._valType) {
  case ValueType::NULL_VALUE:
    os << "nullptr";
    break;
  case ValueType::SOLE_VALUE:
  case ValueType::BYTES_VALUE:
    os << (char *)dv._bysValue;
    break;
  }

  return os;
}
} // namespace storage
