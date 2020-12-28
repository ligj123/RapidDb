#pragma once
#include <unordered_map>
#include "BufferPool.h"
#include <mutex>
#include <queue>

namespace storage {
  using namespace std;
  class CachePool
  {
  public:
    static Byte* Apply(uint32_t eleSize);
    static void Release(Byte* pBuf, uint32_t eleSize);
  public:
    CachePool();
    ~CachePool();
  protected:
    static CachePool* GetInstance() {
      return _gCachePool;
    }
    static Buffer* AllotBuffer(uint32_t eleLen);
    static void RecycleBuffer(Buffer* buf);
    static uint64_t GetMemCacheUsed() { return _gCachePool->_szMemUsed; }
 
  protected:
    static CachePool* _gCachePool;

    uint64_t _szMemUsed;
    unordered_map<uint32_t, BufferPool*> _mapPool;
    queue<Buffer*> _queueFreeBuf;
    utils::SpinMutex _spinMutex;

    friend class BufferPool;
  };
}