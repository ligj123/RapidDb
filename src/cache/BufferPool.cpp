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
Buffer::Buffer(uint32_t eleSize) : _eleSize(eleSize)
{
#ifdef _MSVC_LANG
  _pBuf = (Byte*)_aligned_malloc(Configure::GetCacheBlockSize(), Configure::GetCacheBlockSize());
#else
  _pBuf = (Byte*)aligned_alloc(Configure::GetCacheBlockSize(), Configure::GetCacheBlockSize());
#endif // _MSVC_LANG
  Init(eleSize);
}

void Buffer::Init(uint32_t eleSize)
{
  _eleSize = eleSize;
  _maxEle = (uint32_t)Configure::GetCacheBlockSize() / eleSize;
  _stackFree = stack<uint32_t>();
  for (uint32_t i = 0; i < _maxEle; i++) {
    _stackFree.push(i);
  }
}

Buffer::~Buffer()
{
#ifdef _MSVC_LANG
  _aligned_free(_pBuf);
#else
  free(_pBuf);
#endif // _MSVC_LANG
}

Byte* Buffer::Apply()
{
  assert(!IsEmpty());
  uint32_t index = _stackFree.top();
  _stackFree.pop();
  return &_pBuf[_eleSize * index];
}

void Buffer::Release(Byte* bys)
{
  uint32_t index = (uint32_t)(((uint64_t)bys & (Configure::GetCacheBlockSize() - 1)) / _eleSize);
  _stackFree.push(index);
}

void Buffer::Apply(vector<Byte*>& vct)
{
  assert(!IsEmpty());

  while (vct.size() < vct.capacity() / 2 && !IsEmpty()) {
    uint32_t index = _stackFree.top();
    _stackFree.pop();
    vct.push_back(&_pBuf[_eleSize * index]);
  }
}

void Buffer::Release(vector<Byte*>& vct, bool bAll)
{
  for (int i = (int)vct.size() - 1; i >= 0; i--) {
    Byte* bys = vct[i];
    if ((((uint64_t)bys) ^ ((uint64_t)_pBuf)) > Configure::GetCacheBlockSize()) continue;

    uint32_t index = (uint32_t)(((uint64_t)bys & (Configure::GetCacheBlockSize() - 1)) / _eleSize);
    _stackFree.push(index);

    vct.erase(vct.begin() + i);
    if (vct.size() < vct.capacity() / 2 && !bAll) break;
  }
}

BufferPool::BufferPool(uint32_t eleSize) : _eleSize(eleSize)
{
}

BufferPool::~BufferPool()
{
  for (auto iter = _mapBuffer.begin(); iter != _mapBuffer.end(); iter++)
  {
    delete iter->second;
  }
}

void BufferPool::Apply(vector<Byte*>& vct)
{
  std::unique_lock<utils::SpinMutex> lock(_spinMutex);

  while (vct.size() < vct.capacity() / 2) {
    if (_mapFreeBuffer.size() == 0)
    {
      Buffer* buff = CachePool::AllocateBuffer(_eleSize);
      _mapBuffer.insert(pair<Byte*, Buffer*>(buff->GetBuf(), buff));
      _mapFreeBuffer.insert(pair<Byte*, Buffer*>(buff->GetBuf(), buff));
    }
    auto iter = _mapFreeBuffer.begin();
    iter->second->Apply(vct);
    if (iter->second->IsEmpty()) {
      _mapFreeBuffer.erase(iter);
    }
  }
}

void BufferPool::Release(vector<Byte*>& vct, bool bAll)
{
  std::unique_lock<utils::SpinMutex> lock(_spinMutex);
  int sz = (bAll ? 0 : (int)vct.capacity() / 2);
  while (vct.size() > sz) {
    Byte* buf = Buffer::CalcAddr(vct[0]);
    auto iter = _mapBuffer.find(buf);
    iter->second->Release(vct, bAll);

    if (iter->second->IsFull()) {
      CachePool::RecycleBuffer(iter->second);
      _mapBuffer.erase(iter);
      _mapFreeBuffer.erase(buf);
    }
  }
}

Byte* BufferPool::Apply()
{
  std::unique_lock<utils::SpinMutex> lock(_spinMutex);
  if (_mapFreeBuffer.size() == 0)
  {
    Buffer* buff = CachePool::AllocateBuffer(_eleSize);
    _mapBuffer.insert(pair<Byte*, Buffer*>(buff->GetBuf(), buff));
    _mapFreeBuffer.insert(pair<Byte*, Buffer*>(buff->GetBuf(), buff));
    return buff->Apply();
  } else {
    auto iter = _mapFreeBuffer.begin();
    Byte* bys = iter->second->Apply();
    if (iter->second->IsEmpty()) {
      _mapFreeBuffer.erase(iter);
    }

    return bys;
  }
}

void BufferPool::Release(Byte* bys)
{
  std::unique_lock<utils::SpinMutex> lock(_spinMutex);
  Byte* buf = Buffer::CalcAddr(bys);
  auto iter = _mapBuffer.find(buf);
  iter->second->Release(bys);
  if (iter->second->IsFull())
  {
    CachePool::RecycleBuffer(iter->second);
    _mapBuffer.erase(iter);
    _mapFreeBuffer.erase(buf);
  }
}
}
