#pragma once
#include "../header.h"
#include "CachePool.h"
#include <cstring>

namespace storage {
class StrBuff {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  StrBuff(uint32_t len) : _strLen(0) {
    if (len > 0) {
      _buf = (char *)CachePool::ApplyBys(len, _bufLen);
      _buf[0] = 0;
    } else {
      _buf = nullptr;
      _bufLen = 0;
    }
  }

  StrBuff(uint32_t len, const char *src) {
    _strLen = (uint32_t)strlen(src);
    if (len + 1 < _strLen) {
      len = _strLen + 16;
    }
    _buf = (char *)CachePool::ApplyBys(len, _bufLen);
    strcpy(_buf, src);
  }

  ~StrBuff() {
    if (_buf != nullptr) {
      CachePool::ReleaseBys((Byte *)_buf, _bufLen);
    }
  }

  void Resize(uint32_t len) {
    if (len > _bufLen) {
      char *obuf = _buf;
      uint32_t olen = _bufLen;
      _buf = (char *)CachePool::ApplyBys(len, _bufLen);
      if (obuf != nullptr) {
        strcpy(_buf, obuf);
        CachePool::ReleaseBys((Byte *)obuf, olen);
      }
    }
  }

  void Copy(const char *src) { Copy(src, (uint32_t)strlen(src)); }
  void Copy(const char *src, uint32_t len) {
    if (len + 1 >= _bufLen) {
      char *obuf = _buf;
      uint32_t olen = _bufLen;
      _buf = (char *)CachePool::ApplyBys(len, _bufLen);

      if (obuf != nullptr) {
        CachePool::ReleaseBys((Byte *)obuf, olen);
      }
    }

    _strLen = len;
    strcpy(_buf, src);
  }

  void Cat(const char *src) { Cat(src, (uint32_t)strlen(src)); }
  void Cat(const char *src, uint32_t len) {
    if (len + 1 >= _bufLen) {
      char *obuf = _buf;
      uint32_t olen = _bufLen;
      _buf = (char *)CachePool::ApplyBys(len, _bufLen);

      if (obuf != nullptr) {
        CachePool::ReleaseBys((Byte *)obuf, olen);
        strcpy(_buf, obuf);
      } else {
        _buf[0] = 0;
      }
    }

    _strLen += len;
    strcat(_buf, src);
  }
  uint32_t GetBufLen() { return _bufLen; }
  uint32_t GetStrLen() { return _strLen; }
  uint32_t GetFreeLen() { return _bufLen - _strLen; }
  const char *GetBuff() { return _buf; }
  char *GetFreeBuff() { return _buf + _strLen; }
  void SetStrLen(uint32_t len) { _strLen = len; }
  bool operator==(const StrBuff &sb) const {
    if (_strLen != sb._strLen)
      return false;
    return memcmp(_buf, sb._buf, _strLen);
  }

protected:
  uint32_t _bufLen;
  uint32_t _strLen;
  char *_buf;
};
} // namespace storage
