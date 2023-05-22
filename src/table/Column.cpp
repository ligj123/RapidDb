#include "Column.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../utils/ErrorMsg.h"
#include <cstring>

namespace storage {
/**Write column's information into buffer, this is a 1M byes buffer
 */
uint32_t PhysColumn::WriteData(Byte *pBuf) {
  Byte *p = pBuf;
  *((uint32_t *)p) = (uint32_t)_name.size();
  p += sizeof(uint32_t);
  BytesCopy(p, _name.c_str(), _name.size());
  p += _name.size();

  *((uint32_t *)p) = _index;
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
  BytesCopy(p, _comments.c_str(), _comments.size());
  p += _comments.size();

  *p = (_pDefaultVal == nullptr ? 0 : 1);
  p++;
  if (_pDefaultVal != nullptr) {
    p += _pDefaultVal->WriteData(p);
  }

  return (int32_t)(p - pBuf);
}

uint32_t PhysColumn::ReadData(Byte *pBuf) {
  Byte *p = pBuf;

  uint32_t len = *((uint32_t *)p);
  p += sizeof(uint32_t);
  _name = string((char *)p, len);
  p += len;

  _index = *((uint32_t *)p);
  p += sizeof(uint32_t);

  _dataType = (DataType) * ((uint32_t *)p);
  p += sizeof(uint32_t);

  _bNullable = (*p == 0);
  p++;

  _charset = (Charsets) * ((uint32_t *)p);
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
    p += _pDefaultVal->ReadData(p);
  }

  return (uint32_t)(p - pBuf);
}
} // namespace storage
