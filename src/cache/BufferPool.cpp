#include "BufferPool.h"
#include "../config/Configure.h"
#include "CachePool.h"
#include <malloc.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>

namespace storage {
  Buffer::Buffer(uint32_t eleSize) : _eleSize(eleSize)
  {
    _maxEle = (uint32_t)Configure::GetCacheBlockSize() / eleSize;
#ifdef _MSVC_LANG
    _pBuf = (Byte*)_aligned_malloc(Configure::GetCacheBlockSize(), Configure::GetCacheBlockSize());
#else
    _pBuf = (Byte*)std::aligned_alloc(Configure::GetCacheBlockSize(), Configure::GetCacheBlockSize());
#endif // _MSVC_LANG

    for (uint16_t i = 0; i < _maxEle; i++)
    {
      _setFree.insert(i);
    }
  }

  void Buffer::Init(uint32_t eleSize)
  {
    _setFree.clear();
    _eleSize = eleSize;
    _maxEle = (uint32_t)Configure::GetCacheBlockSize() / eleSize;
    for (uint16_t i = 0; i < _maxEle; i++)
    {
      _setFree.insert(i);
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
    uint16_t index = *_setFree.begin();
    _setFree.erase(index);
    return &_pBuf[_eleSize * index];
  }

  void Buffer::Release(Byte* bys)
  {
    _setFree.insert(CalcPos(bys));
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

  Byte* BufferPool::Apply()
  {
    std::lock_guard<utils::SpinMutex> lock(_spinMutex);
    if (_mapFreeBuffer.size() == 0)
    {
      Buffer* buff = CachePool::AllotBuffer(_eleSize);
      _mapBuffer.insert(pair<Byte*, Buffer*>((Byte*)buff, buff));
      _mapFreeBuffer.insert(pair<Byte*, Buffer*>((Byte*)buff, buff));
      return buff->Apply();
    }
    else
    {
      auto iter = _mapFreeBuffer.begin();
      Byte* bys = iter->second->Apply();
      if (iter->second->IsFull())
      {
        _mapFreeBuffer.erase(iter);
      }

      return bys;
    }
  }

  void BufferPool::Release(Byte* bys)
  {
    std::lock_guard<utils::SpinMutex> lock(_spinMutex);
    Byte* buf = Buffer::CalcAddr(bys);
    auto iter = _mapBuffer.find(buf);
    iter->second->Release(bys);
    if (iter->second->IsEmpty())
    {
      CachePool::RecycleBuffer(iter->second);
      _mapBuffer.erase(iter);
      _mapFreeBuffer.erase(buf);
    }
  }
}