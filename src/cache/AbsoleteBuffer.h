#pragma once
#include "../header.h"
#include "CachePool.h"
#include <atomic>
#include <cassert>

namespace storage {
class AbsoleteBuffer {
public:
  AbsoleteBuffer(Byte *bys, int refCount) : _bys(bys), _refCount(refCount) {}
  bool ReleaseCount(int num) {
    int val = _refCount.fetch_sub(num) - num;
    assert(val >= 0);
    if (val == 0) {
      CachePool::Release(_bys, (uint32_t)Configure::GetCachePageSize());
      delete this;
      return true;
    } else {
      return true;
    }
  }

  bool IsDiffBuff(Byte *buf) const { return (buf != _bys); }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  Byte *_bys;
  std::atomic<int> _refCount;
};
} // namespace storage
