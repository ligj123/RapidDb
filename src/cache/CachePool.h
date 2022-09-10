#pragma once
#include "BufferPool.h"
#include <iostream>
#include <mutex>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>

namespace storage {
using namespace std;
class CachePool;
/**thread local variable to save bytes buffer*/
class LocalMap {
public:
  LocalMap();
  ~LocalMap();
  void Push(Byte *pBuf, uint16_t eleSize);
  Byte *Pop(uint16_t eleSize);

protected:
  unordered_map<uint16_t, vector<Byte *>> _map;
  bool bStoped = false;
};

class CachePool {
public:
#ifdef DEBUG_TEST
  static Byte *ApplyBlock();
  static void ReleaseBlock(Byte *bys);
  static Byte *ApplyPage();
  static void ReleasePage(Byte *page);
  static Byte *Apply(uint32_t bufSize);
  static Byte *Apply(uint32_t bufSize, uint32_t &realSize);
  static void Release(Byte *pBuf, uint32_t bufSize);

  static size_t GetMemoryUsed();
  static vector<unordered_map<uint16_t, vector<Byte *>> *> _vctMap;
  static SpinMutex _spinLocal;
  static unordered_map<Byte *, string> _mapApply;
#else
  /**Apply a memory block for result set*/
  static Byte *ApplyBlock() {
    CachePool *pool = GetInstance();
    unique_lock<SpinMutex> lock(pool->_blockMutex);
    Byte *bys = nullptr;
    if (pool->_queueFreeBlock.size() > 0) {
      bys = pool->_queueFreeBlock.front();
      pool->_queueFreeBlock.pop();
    } else {
      bys = new Byte[Configure::GetResultBlockSize()];
      pool->_totalBlockNum++;
    }

    return bys;
  }
  /**Release a memory block for result set*/
  static void ReleaseBlock(Byte *bys) {
    CachePool *pool = GetInstance();
    unique_lock<SpinMutex> lock(pool->_blockMutex);
    if (pool->_queueFreeBlock.size() >
        Configure::GetMaxNumberFreeResultBlock()) {
      pool->_totalBlockNum--;
      delete bys;
    } else {
      pool->_queueFreeBlock.push(bys);
    }
  }

  /**Apply a menory block for an index page*/
  static Byte *ApplyPage() {
    CachePool *pool = GetInstance();
    return pool->_localMap.Pop((uint16_t)Configure::GetCachePageSize());
  }

  /**Release a memory block for an index page*/
  static void ReleasePage(Byte *page) {
    CachePool *pool = GetInstance();
    pool->_localMap.Push(page, (uint16_t)Configure::GetCachePageSize());
  }

  /**Apply a memory block from cache*/
  static inline Byte *Apply(uint32_t bufSize) {
    uint32_t sz = CalcBufSize(bufSize);
    if (sz == UINT32_MAX)
      return new Byte[bufSize];
    else {
      CachePool *pool = GetInstance();
      return pool->_localMap.Pop(sz);
    }
  }
  /**Apply a memory block from cache and set the actual allocated size*/
  static inline Byte *Apply(uint32_t bufSize, uint32_t &realSize) {
    realSize = CalcBufSize(bufSize);
    if (realSize == UINT32_MAX) {
      realSize = bufSize;
      return new Byte[realSize];
    } else {
      CachePool *pool = GetInstance();
      return pool->_localMap.Pop((uint16_t)realSize);
    }
  }
  /**Release a memory block with unfixed size*/
  static inline void Release(Byte *pBuf, uint32_t bufSize) {
    uint32_t sz = CalcBufSize(bufSize);
    if (sz == UINT32_MAX)
      delete[] pBuf;
    else {
      CachePool *pool = GetInstance();
      pool->_localMap.Push(pBuf, (uint16_t)sz);
    }
  }
#endif // DEBUG_TEST

public:
  CachePool();
  ~CachePool();

protected:
  static CachePool *GetInstance() { return _gCachePool; }
  static Buffer *AllocateBuffer(uint32_t eleLen);
  static void RecycleBuffer(Buffer *buf);
  static uint64_t GetMemCacheUsed() { return _gCachePool->_szMemUsed; }
  static void BatchApply(uint32_t bufSize, vector<Byte *> &vct);
  static void BatchRelease(uint32_t bufSize, vector<Byte *> &vct,
                           bool bAll = false);

protected:
  static uint32_t CalcBufSize(uint32_t sz) {
    if (sz <= 64)
      return ((sz + 15) & 0xFFF0);
    if (sz <= 256)
      return ((sz + 31) & 0xFFE0);
    if (sz > 16384)
      return UINT32_MAX;

    uint32_t x = sz | (sz >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x + 1;
    return ((x >> 1) ^ sz) != 0 ? x : (x >> 1);
  }
  static thread_local LocalMap _localMap;
  static CachePool *_gCachePool;

  /**The totla memory size that has been allocated.*/
  uint64_t _szMemUsed;
  /**To manage the buffer pools for different size*/
  unordered_map<uint32_t, BufferPool *> _mapPool;
  /**Save the free block memory. It will reused next time.*/
  queue<Buffer *> _queueFreeBuf;
  /**Mutex for block memory,used to create IDataValue etc. One block can create
   * multi objects.*/
  SpinMutex _spinMutex;

  /**memory cache block used in IResultSet*/
  queue<Byte *> _queueFreeBlock;
  /**Total number allocated result blocks, include free blocks in queue*/
  uint64_t _totalBlockNum;
  /**Mutex for block*/
  SpinMutex _blockMutex;

  friend class BufferPool;
  friend class LocalMap;
};
} // namespace storage
