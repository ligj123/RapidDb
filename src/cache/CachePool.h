#pragma once
#include <unordered_map>
#include "BufferPool.h"
#include <mutex>
#include <queue>
#include <set>
#include <stack>

namespace storage {
using namespace std;
class CachePool;
/**thread local variable to save bytes buffer*/
class LocalMap {
public:
  ~LocalMap();
  void Push(Byte* pBuf, uint32_t eleSize);
  Byte* Pop(uint32_t eleSize);
protected:
  unordered_map<uint32_t, vector<Byte*>> _map;
};

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
  static inline Byte* ApplyBys(uint32_t bufSize) {
    uint32_t sz = *_gSetBufSize.lower_bound(bufSize);
    return Apply(sz);
  }
  /**Release a memory block with unfixed size*/
  static inline void ReleaseBys(Byte* pBuf, uint32_t bufSize) {
    uint32_t sz = *_gSetBufSize.lower_bound(bufSize);
    return Release(pBuf, sz);
  }

public:
  CachePool();
  ~CachePool();
protected:
  static CachePool* GetInstance() {
    return _gCachePool;
  }
  static Buffer* AllocateBuffer(uint32_t eleLen);
  static void RecycleBuffer(Buffer* buf);
  static uint64_t GetMemCacheUsed() { return _gCachePool->_szMemUsed; }
  static void BatchApply(uint32_t bufSize, vector<Byte*>& vct);
  static void BatchRelease(uint32_t bufSize, vector<Byte*>& vct, bool bAll = false);
protected:
  static thread_local LocalMap _localMap;

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

  friend class BufferPool;
  friend class LocalMap;
};
}
