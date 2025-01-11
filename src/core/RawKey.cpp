#include "RawKey.h"

#include "../utils/Log.h"

namespace storage {
void RawKey::PrintKey(bool bchar) {
  if (bchar) {
    LOG_INFO << (char *)_bysVal;
  } else {
    MString str;
    str.reserve(GetLength() * 2 + 2);
    str.append("0x");
    Byte *bys = _bysVal;
    for (uint32_t i = 0; i < GetLength(); i++) {
      str.append(HexStr[*bys]);
      bys++;
    }

    LOG_INFO << str;
  }
}

} // namespace storage