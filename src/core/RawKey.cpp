#include "RawKey.h"
#include "../utils/BytesConvert.h"
#include <iomanip>

namespace storage {
RawKey::RawKey(VectorDataValue &vctKey) : _bSole(true) {
  _length = 0;
  for (size_t i = 0; i < vctKey.size(); i++) {
    if (vctKey[i]->IsNull()) {
      vctKey[i]->SetDefaultValue();
    }

    _length += vctKey[i]->GetPersistenceLength(true);
  }

  _bysVal = CachePool::ApplyBys(_length);

  int pos = 0;
  for (int i = 0; i < vctKey.size(); i++) {
    pos += vctKey[i]->WriteData(_bysVal + pos, true);
  }
}

RawKey::RawKey(Byte *bys, uint32_t len, bool sole)
    : _bysVal(bys), _length(len), _bSole(sole) {}

RawKey::~RawKey() {
  if (_bSole)
    CachePool::ReleaseBys(_bysVal, _length);
}

bool RawKey::operator>(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length) > 0;
}
bool RawKey::operator<(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length) < 0;
}

bool RawKey::operator>=(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length) >= 0;
}

bool RawKey::operator<=(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length) <= 0;
}

bool RawKey::operator==(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length) == 0;
}

bool RawKey::operator!=(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length) != 0;
}

int RawKey::CompareTo(const RawKey &key) const {
  return utils::BytesCompare(_bysVal, _length, key._bysVal, key._length);
}

std::ostream &operator<<(std::ostream &os, const RawKey &key) {
  os << "Length=" << key._length << std::uppercase << std::hex
     << std::setfill('0') << "\tKey=0x";
  for (uint32_t i = 0; i < key._length; i++) {
    os << std::setw(2) << key._bysVal[i];
    if (i % 4 == 0)
      os << ' ';
  }

  return os;
}
} // namespace storage
