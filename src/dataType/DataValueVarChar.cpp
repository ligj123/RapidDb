#include "DataValueVarChar.h"
#include "../cache/Mallocator.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace storage {

bool DataValueVarChar::SetValue(const char *val, uint32_t len) {
  if (len >= _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len + 1)}));
    return false;
  }

  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType = ValueType::SOLE_VALUE;
  _soleLength = len + 1;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, val, len);
  _bysValue[len] = 0;
  return true;
}

bool DataValueVarChar::PutValue(std::any val) {
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
    _threadErrorMsg.reset(new ErrorMsg(
        DT_UNSUPPORT_CONVERT, {val.type().name(), "DataValueVarChar"}));
    return false;
  }

  if (len == 0)
    len = strlen(buf);

  if (len >= _maxLength) {
    _threadErrorMsg.reset(new ErrorMsg(
        DT_INPUT_OVER_LENGTH, {ToMString(_maxLength), ToMString(len)}));
    return false;
  }

  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType = ValueType::SOLE_VALUE;
  _soleLength = (uint32_t)len + 1;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, buf, _soleLength);
  return true;
}

bool DataValueVarChar::Copy(IDataValue &dv, bool bMove) {
  if (dv.GetRef() > 1) {
    bMove = false;
  };

  if (dv.IsNull()) {
    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _soleLength);
    }

    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
    return true;
  }

  if (!dv.IsStringType()) {
    StrBuff sb(0);
    dv.ToString(sb);
    if (_maxLength < sb.GetStrLen() + 1) {
      _threadErrorMsg.reset(
          new ErrorMsg(DT_INPUT_OVER_LENGTH,
                       {ToMString(_maxLength), ToMString(sb.GetStrLen() + 1)}));
      return false;
    }

    if (_valType == ValueType::SOLE_VALUE) {
      CachePool::Release(_bysValue, _soleLength);
    }

    _bysValue = CachePool::Apply(_maxLength);

    int len = sb.GetStrLen();
    _soleLength = len + 1;
    _valType = ValueType::SOLE_VALUE;
    BytesCopy(_bysValue, sb.GetBuff(), len);
    _bysValue[len] = 0;
    return true;
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

  if (bMove && dv.GetDataType() == DataType::VARCHAR) {
    _bysValue = ((DataValueVarChar &)dv)._bysValue;
    _valType = dv.GetValueType();
    _soleLength = dv.GetDataLength();
    ((DataValueVarChar &)dv)._bysValue = nullptr;
    ((DataValueVarChar &)dv)._valType = ValueType::NULL_VALUE;
  } else if (dv.GetDataType() == DataType::VARCHAR &&
             dv.GetValueType() == ValueType::BYTES_VALUE) {
    _bysValue = ((DataValueVarChar &)dv)._bysValue;
    _valType = ValueType::BYTES_VALUE;
    _soleLength = dv.GetDataLength();
  } else {
    _valType = ValueType::SOLE_VALUE;
    if (dv.GetDataType() == DataType::FIXCHAR) {
      uint32_t len = dv.GetDataLength();
      const Byte *bys = dv.GetBuff();
      while (true) {
        len--;
        if (bys[len] == ' ')
          break;
        if (len == 0)
          break;
      }

      _soleLength = len + 1;
      _bysValue = CachePool::Apply(_soleLength);
      BytesCopy(_bysValue, ((DataValueVarChar &)dv)._bysValue, len);
      _bysValue[len] = 0;
    } else {
      _soleLength = dv.GetDataLength();
      _bysValue = CachePool::Apply(_soleLength);
      BytesCopy(_bysValue, ((DataValueVarChar &)dv)._bysValue, _soleLength);
    }
  }

  return true;
}

uint32_t DataValueVarChar::WriteData(Byte *buf, SavePosition svPos) const {
  if (svPos == SavePosition::KEY) {
    // Write default value if is null for key
    if (_valType == ValueType::NULL_VALUE) {
      *buf = 0;
      return 1;
    }

    BytesCopy(buf, _bysValue, _soleLength);
    if (_soleLength < _maxLength) {
      buf[_soleLength] = '\0';
      return _soleLength + 1;
    } else {
      return _maxLength;
    }
  } else {
    if (_valType == ValueType::NULL_VALUE) {
      return 0;
    } else {
      BytesCopy(buf, _bysValue, _soleLength);
      return _soleLength;
    }
  }
}

uint32_t DataValueVarChar::ReadData(const Byte *buf, uint32_t len,
                                    SavePosition svPos, bool bSole) {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  assert(svPos == SavePosition::VALUE);
  if (len == 0) {
    _valType = ValueType::NULL_VALUE;
    _bysValue = nullptr;
    return 0;
  }

  assert(len <= _maxLength);
  _soleLength = len;
  if (bSole) {
    _bysValue = CachePool::Apply(_soleLength);
    BytesCopy(_bysValue, buf, _soleLength);
    _valType = ValueType::SOLE_VALUE;
  } else {
    _bysValue = const_cast<Byte *>(buf);
    _valType = ValueType::BYTES_VALUE;
  }
  return _soleLength;
}

uint32_t DataValueVarChar::WriteData(Byte *buf) const {
  if (_valType == ValueType::NULL_VALUE) {
    buf[0] = (Byte)DataType::VARCHAR & DATE_TYPE;
    return 1;
  } else {
    buf[0] = (VALUE_TYPE | ((Byte)DataType::VARCHAR & DATE_TYPE));
    BytesCopy(buf + 1, (Byte *)&_soleLength, sizeof(uint32_t));
    BytesCopy(buf + 1 + sizeof(uint32_t), (char *)_bysValue, _soleLength);
    return _soleLength + sizeof(uint32_t) + 1;
  }
}

uint32_t DataValueVarChar::ReadData(const Byte *buf) {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }
  _valType =
      (buf[0] & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
  if (_valType == ValueType::NULL_VALUE) {
    _valType = ValueType::NULL_VALUE;
    return 1;
  }

  _valType = ValueType::SOLE_VALUE;
  BytesCopy((char *)&_soleLength, buf + 1, sizeof(uint32_t));
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, buf + 1 + sizeof(uint32_t), _soleLength);
  return _soleLength + sizeof(uint32_t) + 1;
}

void DataValueVarChar::SetMinValue() {
  if (_valType == ValueType::SOLE_VALUE)
    CachePool::Release(_bysValue, _soleLength);

  _valType = ValueType::SOLE_VALUE;
  _soleLength = 1;
  _bysValue = CachePool::Apply(_soleLength);
  _bysValue[0] = 0;
}

void DataValueVarChar::SetMaxValue() {
  if (_valType == ValueType::SOLE_VALUE) {
    CachePool::Release(_bysValue, _soleLength);
  }

  _valType = ValueType::SOLE_VALUE;
  _soleLength = _maxLength;
  _bysValue = CachePool::Apply(_soleLength);
  memset(_bysValue, UINT8_MAX, _soleLength);
}

void DataValueVarChar::SetDefaultValue() {
  if (_valType == ValueType::SOLE_VALUE)
    CachePool::Release(_bysValue, _soleLength);

  _valType = ValueType::SOLE_VALUE;
  _soleLength = 1;
  _bysValue = CachePool::Apply(_soleLength);
  _bysValue[0] = 0;
}

DataValueVarChar &DataValueVarChar::operator=(const char *val) {
  uint32_t len = (uint32_t)strlen(val);
  if (len >= _maxLength - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {ToMString(_maxLength), ToMString(_soleLength)});
  if (_valType == ValueType::SOLE_VALUE)
    CachePool::Release(_bysValue, _soleLength);

  _soleLength = len;
  _valType = ValueType::SOLE_VALUE;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, val, _soleLength);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const MString val) {
  uint32_t len = (uint32_t)val.size();
  if (len >= _maxLength - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {ToMString(_maxLength), ToMString(_soleLength)});
  if (_valType == ValueType::SOLE_VALUE)
    CachePool::Release(_bysValue, _soleLength);

  _soleLength = len;
  _valType = ValueType::SOLE_VALUE;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, val.c_str(), _soleLength);
  return *this;
}

DataValueVarChar &DataValueVarChar::operator=(const string val) {
  uint32_t len = (uint32_t)val.size();
  if (len >= _maxLength - 1)
    throw ErrorMsg(DT_INPUT_OVER_LENGTH,
                   {ToMString(_maxLength), ToMString(_soleLength)});
  if (_valType == ValueType::SOLE_VALUE)
    CachePool::Release(_bysValue, _soleLength);

  _soleLength = len;
  _valType = ValueType::SOLE_VALUE;
  _bysValue = CachePool::Apply(_soleLength);
  BytesCopy(_bysValue, val.c_str(), _soleLength);
  return *this;
}

std::ostream &operator<<(std::ostream &os, const DataValueVarChar &dv) {
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
