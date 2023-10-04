#include "Column.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../utils/ErrorMsg.h"
#include <cstring>

namespace storage {
uint32_t PhysColumn::CalcSize() {
  uint32_t len = UI16_LEN + (uint32_t)_name.size();
  len += UI32_LEN;
  len += 1 + UI16_LEN + UI32_LEN + UI64_LEN + UI64_LEN;
  len += UI16_LEN + (uint32_t)_comments.size();
  len += 2 + (_pDefaultVal == nullptr
                  ? 0
                  : _pDefaultVal->GetPersistenceLength(SavePosition::VALUE));
  return len;
}

/**Write column's information into buffer, this is a 1M byes buffer
 */
uint32_t PhysColumn::WriteData(Byte *pBuf) {
  Byte *p = pBuf;
  *((uint16_t *)p) = (uint16_t)_name.size();
  p += UI16_LEN;
  BytesCopy(p, _name.c_str(), _name.size());
  p += _name.size();

  *((uint32_t *)p) = (uint32_t)_dataType;
  p += UI32_LEN;

  *p = (_bNullable ? 0 : 1);
  p++;

  *((uint16_t *)p) = (uint16_t)_charset;
  p += UI16_LEN;

  *((uint32_t *)p) = _maxLength;
  p += UI32_LEN;

  *((uint64_t *)p) = _initVal;
  p += UI64_LEN;

  *((uint64_t *)p) = _incStep;
  p += UI64_LEN;

  *((uint16_t *)p) = (uint16_t)_comments.size();
  p += UI16_LEN;
  BytesCopy(p, _comments.c_str(), _comments.size());
  p += _comments.size();

  *((uint16_t *)p) =
      (_pDefaultVal == nullptr
           ? 0
           : _pDefaultVal->GetPersistenceLength(SavePosition::VALUE));
  p += UI16_LEN;
  if (_pDefaultVal != nullptr) {
    p += _pDefaultVal->WriteData(p, SavePosition::VALUE);
  }

  return (int32_t)(p - pBuf);
}

uint32_t PhysColumn::ReadData(Byte *pBuf) {
  if (_pDefaultVal != nullptr) {
    _pDefaultVal->DecRef();
    _pDefaultVal = nullptr;
  }

  Byte *p = pBuf;
  uint16_t len = *((uint16_t *)p);
  p += UI16_LEN;
  _name = MString((char *)p, len);
  p += len;

  _dataType = (DataType) * ((uint32_t *)p);
  p += UI32_LEN;

  _bNullable = (*p == 0);
  p++;

  _charset = (Charsets) * ((uint16_t *)p);
  p += UI16_LEN;

  _maxLength = *((uint32_t *)p);
  p += UI32_LEN;

  _initVal = *((uint64_t *)p);
  p += UI64_LEN;

  _incStep = *((uint64_t *)p);
  p += UI64_LEN;

  len = *((uint16_t *)p);
  p += UI16_LEN;
  _comments = MString((char *)p, len);
  p += len;

  len = *((uint16_t *)p);
  p += UI16_LEN;
  if (len > 0) {
    _pDefaultVal = DataValueFactory(_dataType, len);
    _pDefaultVal->SetConstRef();
    p += _pDefaultVal->ReadData(p, len, SavePosition::VALUE, true);
  }

  return (uint32_t)(p - pBuf);
}
} // namespace storage
