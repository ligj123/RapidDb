#include "BufferPool.h"
#include "../config/Configure.h"
#include "CachePool.h"
#ifdef _MSVC_LANG
#include <malloc.h>
#include <stdio.h>
#else
#include <cstdio>
#include <cstdlib>
#endif // _MSVC_LANG

namespace storage {
const uint64_t Buffer::BUFFER_MASK = Configure::GetCacheBlockSize() - 1;
const uint64_t Buffer::BUFFER_MASK_N = ~(Configure::GetCacheBlockSize() - 1);

Buffer::Buffer(uint16_t eleSize) : _eleSize(eleSize) {
#ifdef _MSVC_LANG
  _pBuf = (Byte *)_aligned_malloc(Configure::GetCacheBlockSize(),
                                  Configure::GetCacheBlockSize());
#else
  _pBuf = (Byte *)aligned_alloc(Configure::GetCacheBlockSize(),
                                Configure::GetCacheBlockSize());
#endif // _MSVC_LANG
  Init(eleSize);
}

void Buffer::Init(uint16_t eleSize) {
  _eleSize = eleSize;
  _maxEle = (uint32_t)Configure::GetCacheBlockSize() / eleSize;
  _vctFree.clear();
  _vctFree.reserve(_maxEle);
  for (uint16_t i = 0; i < _maxEle; i++) {
    _vctFree.push_back(i);
  }
}

Buffer::~Buffer() {
#ifdef _MSVC_LANG
  _aligned_free(_pBuf);
#else
  free(_pBuf);
#endif // _MSVC_LANG
}

Byte *Buffer::Apply() {
  assert(!IsEmpty());
  auto iter = _vctFree.end() - 1;
  uint16_t index = *iter;
  _vctFree.erase(iter);
  return &_pBuf[_eleSize * index];
}

void Buffer::Release(Byte *bys) {
  uint16_t index = (uint16_t)(((uint64_t)bys & BUFFER_MASK) / _eleSize);
  _vctFree.push_back(index);
}

void Buffer::Apply(vector<Byte *> &vct) {
  assert(!IsEmpty());
  size_t cap = (vct.capacity() >> 2) + (vct.capacity() >> 1);
  while (vct.size() < cap && !IsEmpty()) {
    auto iter = _vctFree.end() - 1;
    uint16_t index = *iter;
    _vctFree.erase(iter);
    vct.push_back(&_pBuf[_eleSize * index]);
  }
}

void Buffer::Release(vector<Byte *> &vct, bool bAll) {
  size_t cap = (vct.capacity() >> 2);
  for (int i = (int)vct.size() - 1; i >= 0; i--) {
    Byte *bys = vct[i];
    if ((((uint64_t)bys) ^ ((uint64_t)_pBuf)) > Configure::GetCacheBlockSize())
      continue;

    uint16_t index = (uint16_t)(((uint64_t)bys & BUFFER_MASK) / _eleSize);
    _vctFree.push_back(index);

    vct.erase(vct.begin() + i);
    if (!bAll && vct.size() < cap)
      break;
  }
}

BufferPool::BufferPool(uint16_t eleSize) : _eleSize(eleSize) {}

BufferPool::~BufferPool() {
  for (auto iter = _mapBuffer.begin(); iter != _mapBuffer.end(); iter++) {
    delete iter->second;
  }
}

void BufferPool::Apply(vector<Byte *> &vct) {
  std::unique_lock<SpinMutex> lock(_spinMutex);
  size_t cap = (vct.capacity() >> 2) + (vct.capacity() >> 1);
  while (vct.size() < cap) {
    unordered_map<Byte *, storage::Buffer *>::iterator iter;
    if (_mapFreeBuffer.size() == 0) {
      Buffer *buff = CachePool::AllocateBuffer(_eleSize);
      _mapBuffer.insert(pair<Byte *, Buffer *>(buff->GetBuf(), buff));
      iter = _mapFreeBuffer.insert(pair<Byte *, Buffer *>(buff->GetBuf(), buff))
                 .first;
    } else {
      iter = _mapFreeBuffer.begin();
    }

    iter->second->Apply(vct);
    if (iter->second->IsEmpty()) {
      _mapFreeBuffer.erase(iter);
    }
  }
}

void BufferPool::Release(vector<Byte *> &vct, bool bAll) {
  std::unique_lock<SpinMutex> lock(_spinMutex);
  size_t sz = (bAll ? 0 : (vct.capacity() >> 2));
  while (vct.size() > sz) {
    Byte *buf = Buffer::CalcAddr(vct[0]);
    auto iter = _mapBuffer.find(buf);
    if (iter->second->IsEmpty()) {
      _mapFreeBuffer.insert({buf, iter->second});
    }

    iter->second->Release(vct, bAll);

    if (iter->second->IsFull()) {
      CachePool::RecycleBuffer(iter->second);
      _mapBuffer.erase(iter);
      _mapFreeBuffer.erase(buf);
    }
  }
}

Byte *BufferPool::Apply() {
  std::unique_lock<SpinMutex> lock(_spinMutex);
  if (_mapFreeBuffer.size() == 0) {
    Buffer *buff = CachePool::AllocateBuffer(_eleSize);
    _mapBuffer.insert(pair<Byte *, Buffer *>(buff->GetBuf(), buff));
    _mapFreeBuffer.insert(pair<Byte *, Buffer *>(buff->GetBuf(), buff));
    return buff->Apply();
  } else {
    auto iter = _mapFreeBuffer.begin();
    Byte *bys = iter->second->Apply();
    if (iter->second->IsEmpty()) {
      _mapFreeBuffer.erase(iter);
    }

    return bys;
  }
}

void BufferPool::Release(Byte *bys) {
  std::unique_lock<SpinMutex> lock(_spinMutex);
  Byte *buf = Buffer::CalcAddr(bys);
  auto iter = _mapBuffer.find(buf);
  iter->second->Release(bys);
  if (iter->second->IsFull()) {
    CachePool::RecycleBuffer(iter->second);
    _mapBuffer.erase(iter);
    _mapFreeBuffer.erase(buf);
  }
}
} // namespace storage
