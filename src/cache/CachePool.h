#pragma once
#include <unordered_map>
#include "BufferPool.h"
#include <mutex>
#include <queue>
#include <set>

namespace storage {
  using namespace std;
  class CachePool
  {
  public:
    static Byte* ApplyPage();
    static void ReleasePage(Byte* page);
    static Byte* Apply(uint32_t eleSize);
    static void Release(Byte* pBuf, uint32_t eleSize);
    static Byte* ApplyBys(uint32_t bufSize);
    static void ReleaseBys(Byte* pBuf, uint32_t bufSize);
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
    static set<uint32_t> _gSetBufSize;
    static CachePool* _gCachePool;

    /**The totla memory size that has been allocated.*/
    uint64_t _szMemUsed;
    /**To manage the buffer pools for different size*/
    unordered_map<uint32_t, BufferPool*> _mapPool;
    /**Save the free block memory. It will reused next time.*/
    queue<Buffer*> _queueFreeBuf;
    /**Mutex for block memory,used to create IDataValue etc. One block can create multi objects.*/
    utils::SpinMutex _spinMutex;

    /**queue for cache page*/
    queue<Byte*> _queueFreePage;
    /**Mutex for cache page */
    utils::SpinMutex _spinMutexPage;

    friend class BufferPool;
  };
}