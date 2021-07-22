﻿#include "Column.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueLong.h"
#include "../dataType/DataValueVarChar.h"
#include "../utils/ErrorMsg.h"
#include <cstring>

namespace storage {
uint32_t PersistColumn::WriteData(Byte *pBuf) {
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

  *((uint32_t *)p) = (uint32_t)_charset;
  p += sizeof(uint32_t);

  *((uint32_t *)p) = _maxLength;
  p += sizeof(uint32_t);

  *((uint64_t *)p) = _initVal;
  p += sizeof(uint64_t);

  *((uint64_t *)p) = _incStep;
  p += sizeof(uint64_t);

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

uint32_t PersistColumn::ReadData(Byte *pBuf) {
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

  _charset = (utils::Charsets) * ((uint32_t *)p);
  p += sizeof(uint32_t);

  _maxLength = *((uint32_t *)p);
  p += sizeof(uint32_t);

  _initVal = *((uint64_t *)p);
  p += sizeof(uint64_t);

  _incStep = *((uint64_t *)p);
  p += sizeof(uint64_t);

  len = *((uint32_t *)p);
  p += sizeof(uint32_t);
  _comments = string((char *)p, len);
  p += len;

  bool bDefault = (*p != 0);
  p++;
  if (bDefault) {
    _pDefaultVal = DataValueFactory(_dataType);
    p += _pDefaultVal->ReadData(p, -1);
  }

  return (uint32_t)(p - pBuf);
}

int MiddleColumn::GetLength(Byte *bys) {
  if ((bys[UI32_LEN + _position / BYTE_SIZE] &
       (Byte)(1 << (_position % BYTE_SIZE))) == 0) {
    return 0;
  }

  switch (_dataType) {
  case DataType::LONG:
  case DataType::ULONG:
  case DataType::DOUBLE:
  case DataType::DATETIME:
    return 8;
  case DataType::FLOAT:
  case DataType::INT:
  case DataType::UINT:
    return 4;
  case DataType::SHORT:
  case DataType::USHORT:
    return 2;
  case DataType::BOOL:
  case DataType::CHAR:
  case DataType::BYTE:
    return 1;
  case DataType::FIXCHAR:
  case DataType::VARCHAR:
  case DataType::BLOB: {
    int pos = sizeof(int32_t) + _colNullPlace;
    return (
        _prevVarCols <= 0
            ? UInt32FromBytes(bys + pos)
            : UInt32FromBytes(bys + pos + _prevVarCols * UI32_LEN) -
                  UInt32FromBytes(bys + pos + (_prevVarCols - 1) * UI32_LEN));
  }
  default:
    return -1;
  }
}

int MiddleColumn::CompareTo(Byte *bys1, Byte *bys2) {
  int pos1 = CalcPosition(bys1);
  int pos2 = CalcPosition(bys2);
  if (pos1 < 0) {
    return -1;
  } else if (pos2 < 0) {
    return 1;
  }

  switch (_dataType) {
  case DataType::LONG:
    return (int)(*((int64_t *)bys1) - *((int64_t *)bys2));
  case DataType::ULONG:
    return (int)(*((uint64_t *)bys1) - *((uint64_t *)bys2));
  case DataType::DOUBLE:
    return (int)(*((double *)bys1) - *((double *)bys2));
  case DataType::DATETIME:
    return (int)(*((uint64_t *)bys1) - *((uint64_t *)bys2));
  case DataType::FLOAT:
    return (int)(*((float *)bys1) - *((float *)bys2));
  case DataType::INT:
    return *((int32_t *)bys1) - *((int32_t *)bys2);
  case DataType::UINT:
    return *((uint32_t *)bys1) - *((uint32_t *)bys2);
  case DataType::SHORT:
    return *((int16_t *)bys1) - *((int16_t *)bys2);
  case DataType::USHORT:
    return *((uint16_t *)bys1) - *((uint16_t *)bys2);
  case DataType::BOOL:
    return (*bys1 ? 1 : 0) - (*bys2 ? 1 : 0);
  case DataType::CHAR:
    return *((char *)bys1) - *((char *)bys2);
  case DataType::BYTE:
    return *bys1 - *bys2;
  case DataType::FIXCHAR:
  case DataType::VARCHAR:
  case DataType::BLOB:
    return BytesCompare(bys1, GetLength(bys1), bys2, GetLength(bys2));
  default:
    throw new ErrorMsg(DT_UNKNOWN_TYPE, {to_string((uint32_t)_dataType)});
  }
}
} // namespace storage
