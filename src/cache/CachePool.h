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
    /**Apply a menory block for an Index page*/
    static Byte* ApplyPage();
    /**Release an memory block for an index page*/
    static void ReleasePage(Byte* page);
    /**Apply a memory block with fixed size*/
    static Byte* Apply(uint32_t eleSize);
    /**Release a memory block with fixed size*/
    static void Release(Byte* pBuf, uint32_t eleSize);
    /**Apply a memory block with unfixed size*/
    static Byte* ApplyBys(uint32_t bufSize);
    /**Release a memory block with unfixed size*/
    static void ReleaseBys(Byte* pBuf, uint32_t bufSize);
    /**Apply a memory block for overflow file cache*/
    static Byte* ApplyOverflowCache();
    /**Release a memory block for overflow file cache*/
    static void ReleaseOverflowCache(Byte* pBuf);
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

    /**queue for index page*/
    queue<Byte*> _queueFreePage;
    /**Mutex for index page */
    utils::SpinMutex _spinMutexPage;

    /**queue for overflow file cache*/
    queue<Byte*> _queueFreeOvf;
    /**Mutex for overflow file cache*/
    utils::SpinMutex _spinMutexOvf;
    friend class BufferPool;
  };
}