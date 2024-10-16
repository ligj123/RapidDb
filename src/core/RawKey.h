﻿#pragma once
#include "../cache/CachePool.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include <cstdint>
#include <cstring>
#include <iomanip>

namespace storage {
class RawKey {
public:
  RawKey() : _bysVal(nullptr), _length(0), _bSole(false) {}
  RawKey(VectorDataValue &vctKey) : _bSole(true) {
    _length = 0;
    for (size_t i = 0; i < vctKey.size(); i++) {
      _length += vctKey[i]->GetPersistenceLength(SavePosition::KEY);
    }

    _bysVal = CachePool::Apply(_length);

    int pos = 0;
    for (int i = 0; i < vctKey.size(); i++) {
      pos += vctKey[i]->WriteData(_bysVal + pos, SavePosition::KEY);
    }
  }
  RawKey(Byte *bys, uint32_t len, bool sole = false)
      : _bysVal(bys), _length(len), _bSole(sole) {}

  RawKey(uint32_t len, const Byte *bys) : _length(len), _bSole(false) {
    _bysVal = CachePool::Apply(_length);
    BytesCopy(_bysVal, bys, _length);
  }
  RawKey(RawKey &&src)
      : _bysVal(src._bysVal), _length(src._length), _bSole(src._bSole) {
    src._bysVal = nullptr;
    src._length = 0;
  }
  RawKey(const RawKey &src) = delete;
  RawKey &operator=(RawKey &&src) {
    _bysVal = src._bysVal;
    _length = src._length;
    _bSole = src._bSole;
    src._bysVal = nullptr;
    src._length = 0;
    return *this;
  }
  RawKey &operator=(const RawKey &src) = delete;

  void Copy(Byte *bys, uint32_t len) {
    if (_bSole) {
      CachePool::Release(_bysVal, _length);
    }
    _length = len;
    _bysVal = CachePool::Apply(len);
    memcpy(_bysVal, bys, len);
    _bSole = true;
  }

  ~RawKey() {
    if (_bSole && _bysVal != nullptr)
      CachePool::Release(_bysVal, _length);
  }

  Byte *GetBysVal() const { return _bysVal; }
  uint32_t GetLength() const { return _length; }

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }

  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  bool operator>(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length) > 0;
  }
  bool operator<(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length) < 0;
  }
  bool operator>=(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length) >= 0;
  }
  bool operator<=(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length) <= 0;
  }
  bool operator==(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length) == 0;
  }
  bool operator!=(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length) != 0;
  }
  int CompareTo(const RawKey &key) const {
    return BytesCompare(_bysVal, _length, key._bysVal, key._length);
  }

protected:
  Byte *_bysVal;
  uint32_t _length;
  bool _bSole;

  friend std::ostream &operator<<(std::ostream &os, const RawKey &key);
};

inline std::ostream &operator<<(std::ostream &os, const RawKey &key) {
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