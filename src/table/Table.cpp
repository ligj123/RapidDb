#include "Table.h"
#include <boost/crc.hpp>

namespace storage {
uint32_t IndexProp::CalcSize() {
  uint32_t sz = UI32_LEN * 2;
  sz += UI16_LEN + _name.size();
  sz += UI16_LEN + 1 + UI16_LEN + UI16_LEN * 2 * _vctCol.size();

  for (IndexColumn col : _vctCol) {
    sz += col.colName.size();
  }
}

void IndexProp::Write(Byte *bys, uint32_t &bysLen) {
  Byte *tmp = bys + UI32_LEN * 2;
  *(uint16_t *)tmp = (uint16_t)_name.size();
  tmp += UI16_LEN;
  BytesCopy(tmp, _name.c_str(), _name.size());
  tmp += _name.size();

  *(uint16_t *)tmp = _position;
  tmp += UI16_LEN;
  *tmp = (Byte)_type;
  tmp++;

  *(uint16_t *)tmp = (uint16_t)_vctCol.size();
  tmp += UI16_LEN;
  for (IndexColumn col : _vctCol) {
    *(uint16_t *)tmp = (uint16_t)col.colName.size();
    tmp += UI16_LEN;
    BytesCopy(tmp, col.colName.c_str(), col.colName.size());
    tmp += col.colName.size();
    *(uint16_t *)tmp = col.colPos;
    tmp += UI16_LEN;
  }

  assert(*(uint32_t *)bys < bysLen);
  *(uint32_t *)(bys + UI32_LEN) = (uint32_t)(tmp - bys);
  boost::crc_32_type crc32;
  crc32.process_bytes(bys + UI32_LEN, tmp - bys - UI32_LEN);
  *(uint32_t *)bys = crc32.checksum();

  bysLen = (uint32_t)(tmp - bys);
}

bool IndexProp::Read(Byte *bys) {
  uint32_t len = *(uint32_t *)(bys + UI32_LEN);
  boost::crc_32_type crc32;
  crc32.process_bytes(bys + UI32_LEN, len);
  if (*(uint32_t *)bys != crc32.checksum())
    return false;

  bys += UI32_LEN * 2;
  uint16_t sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  _name = string((char *)bys, sz);
  bys += sz;

  _position = *(uint16_t *)bys;
  bys += UI16_LEN;
  _type = (IndexType)(int8_t)bys;
  bys++;

  sz = *(uint16_t *)bys;
  bys += UI16_LEN;
  for (uint16_t i = 0; i < sz; i++) {
    IndexColumn col;
    uint16_t len = *(uint16_t *)bys;
    bys += UI16_LEN;

    col.colName = string((char *)bys, len);
    bys += len;
    col.colPos = *(uint16_t *)bys;
    bys += UI16_LEN;
  }

  return true;
}

} // namespace storage