#include "Column.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueLong.h"
#include "../dataType/DataValueVarChar.h"
#include <cstring>

namespace storage {
uint32_t PersistColumn::ReadData(Byte *pBuf) {
  Byte *p = pBuf;
  *((uint32_t *)p) = (uint32_t)_name.size();
  p += sizeof(uint32_t);
  std::memcpy(p, _name.c_str(), _name.size());
  p += _name.size();

  *((uint32_t *)p) = _position;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = (uint32_t)_dataType;
  p += sizeof(uint32_t);

  *p = (_bNullable ? 0 : 1);
  p++;

  *((uint32_t *)p) = _maxLength;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = _incStep;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = (uint32_t)_charset;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = (uint32_t)_comments.size();
  p += sizeof(uint32_t);
  std::memcpy(p, _comments.c_str(), _comments.size());
  p += _comments.size();

  *p = (_pDefaultVal == nullptr ? 0 : 1);
  p++;
  if (_pDefaultVal != nullptr) {
    p += _pDefaultVal->WriteData(p);
  }

  return (int32_t)(p - pBuf);
}

uint32_t PersistColumn::WriteData(Byte *pBuf) {
  Byte *p = pBuf;

  uint32_t len = *((uint32_t *)p);
  p += sizeof(uint32_t);
  _name = string((char *)p, len);
  p += len;

  _position = *((uint32_t *)p);
  p += sizeof(uint32_t);

  _dataType = (DataType) * ((uint32_t *)p);
  p += sizeof(uint32_t);

  _bNullable = (*p == 0);
  p++;

  _maxLength = *((uint32_t *)p);
  p += sizeof(uint32_t);

  _incStep = *((uint32_t *)p);
  p += sizeof(uint32_t);

  _charset = (utils::Charsets) * ((uint32_t *)p);
  p += sizeof(uint32_t);

  len = *((uint32_t *)p);
  p += sizeof(uint32_t);
  _comments = string((char *)p, len);
  p += len;

  bool bDefault = (*p != 0);
  p++;
  if (bDefault) {
    switch (_dataType) {
    case DataType::LONG:
      _pDefaultVal = new DataValueLong(*((int64_t *)p));
      p += sizeof(uint64_t);
      break;
    case DataType::VARCHAR:
      len = (*(int32_t *)p);
      p += sizeof(uint32_t);
      _pDefaultVal = new DataValueVarChar((char *)p);
      p += len;
      break;
    case DataType::FIXCHAR:
      if (p[_maxLength - 1] == '\0')
        _pDefaultVal = new DataValueFixChar((char *)p);
      else
        _pDefaultVal = new DataValueFixChar((char *)p, _maxLength);
      p += _maxLength;
    default:
      break;
    }
  }

  return (uint32_t)(p - pBuf);
}

} // namespace storage
