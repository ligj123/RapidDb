#pragma once
#include "../config/Configure.h"
#include "BufferPool.h"

#include <array>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define PAGE_POS 27

namespace storage {
using namespace std;
extern uint16_t _arrSzMap[];

// Memory status
enum class MemoryStatus : int8_t {
  AMPLE = 0, // The used memory is less than 70%, do not release index page.
  SCARE,     // The used memory is less than 90% and more than 70%, the unused
             // memory should be released.
  CRITICAL,  // The used memory has exceed 90%, LeafPage and other block should
             // be released soon after used.
  FATAL // The used memory has exceed the assigned value, can not assign new
        // memory.
};

class CachePool;
/**thread local variable to save bytes buffer*/
class LocalMap {
public:
  LocalMap();
  ~LocalMap();
  void Push(Byte *pBuf, uint16_t pos);
  Byte *Pop(uint16_t pos);
  int64_t GetMemStat() { return _memStat; }

protected:
  // The array to save the applied memory and not allocated to end user. Every
  // size type occupied a vector.
  array<vector<Byte *>, 28> _arrVctByte;
  // To save this thread allocated and recycled how many memory
  int64_t _memStat{0};
  // The thread is running or not.
  bool bStoped = false;
};

class CachePool {
public:
  static MemoryStatus GetMemoryStatus() {
    int64_t total_mem = Configure::GetTotalMemorySize() / 100;
    int64_t used_mem = GetMemoryUsed();
    if (total_mem * 70 > used_mem)
      return MemoryStatus::AMPLE;
    else if (total_mem * 90 > used_mem)
      return MemoryStatus::SCARE;
    else if (total_mem * 100 > used_mem)
      return MemoryStatus::CRITICAL;
    else
      return MemoryStatus::FATAL;
  }

  static int64_t GetMemoryUsed();
  static int64_t GetMemoryAllocated();

#ifdef CACHE_TRACE
  static Byte *ApplyBlock();
  static void ReleaseBlock(Byte *bys);
  static Byte *ApplyPage();
  static void ReleasePage(Byte *page);
  static Byte *Apply(uint32_t bufSize);
  static Byte *Apply(uint32_t bufSize, uint32_t &realSize);
  static void Release(Byte *pBuf, uint32_t bufSize);

  static SpinMutex _spinTrace;
  static unordered_map<uint64_t, string> _mapApply;
  static bool _bWriteLog;
#else
  /**Apply a memory block for result set*/
  static Byte *ApplyBlock() {
    CachePool *pool = GetInstance();
    unique_lock<SpinMutex> lock(pool->_spinMutex);
    Byte *bys = nullptr;
    if (pool->_vctFreeBlock.size() > 0) {
      bys = pool->_vctFreeBlock.back();
      pool->_vctFreeBlock.pop_back();
    } else {
      bys =
          reinterpret_cast<Byte *>(std::malloc(Configure::GetResultPageSize()));
      pool->_totalBlockNum++;
    }

    return bys;
  }
  /**Release a memory block for result set*/
  static void ReleaseBlock(Byte *bys) {
    CachePool *pool = GetInstance();
    unique_lock<SpinMutex> lock(pool->_spinMutex);
    if (pool->_vctFreeBlock.size() > Configure::GetMaxFreeResultBlock()) {
      pool->_totalBlockNum--;
      std::free(bys);
    } else {
      pool->_vctFreeBlock.push_back(bys);
    }
  }

  /**Apply a menory block for an index page*/
  static Byte *ApplyPage() { return _localMap.Pop(PAGE_POS); }

  /**Release a memory block for an index page*/
  static void ReleasePage(Byte *page) { _localMap.Push(page, PAGE_POS); }

  /**Apply a memory block from cache*/
  static inline Byte *Apply(uint32_t bufSize) {
    uint32_t pos = CalcBufSize(bufSize);
    if (pos == UINT32_MAX) {
      return MallocLargeBlock(bufSize);
    } else {
      return _localMap.Pop(pos);
    }
  }
  /**Apply a memory block from cache and set the actual allocated size*/
  static inline Byte *Apply(uint32_t bufSize, uint32_t &realSize) {
    uint32_t pos = CalcBufSize(bufSize);
    if (pos == UINT32_MAX) {
      realSize = bufSize;
      return MallocLargeBlock(bufSize);
    } else {
      realSize = _arrSzMap[pos];
      return _localMap.Pop(pos);
    }
  }
  /**Release a memory block with unfixed size*/
  static inline void Release(Byte *pBuf, uint32_t bufSize) {
    uint32_t pos = CalcBufSize(bufSize);
    if (pos == UINT32_MAX) {
      FreeLargeBlock(pBuf, bufSize);
    } else {
#ifndef NDEBUG
      memset(pBuf, 0, bufSize);
#endif
      _localMap.Push(pBuf, pos);
    }
  }
#endif // CACHE_TRACE

public:
  CachePool();
  ~CachePool();

  static CachePool *GetInstance() { return _gCachePool; }
  static Buffer *AllocateBuffer(uint32_t eleLen);
  static void RecycleBuffer(Buffer *buf);
  static void BatchApply(uint32_t pos, vector<Byte *> &vct);
  static void BatchRelease(uint32_t pos, vector<Byte *> &vct,
                           bool bAll = false);

  static inline uint32_t CalcBufSize(uint32_t sz) {
    if (sz <= 64)
      return (sz - 1) >> 4;
    else if (sz <= 256)
      return ((sz - 1) >> 5) + 2;
    else if (sz <= 1024)
      return ((sz - 1) >> 7) + 8;
    else if (sz <= 4096)
      return ((sz - 1) >> 9) + 14;
    else if (sz <= 16384)
      return ((sz - 1) >> 11) + 20;
    else
      return UINT32_MAX;
  }

  static Byte *MallocLargeBlock(uint32_t bufsize);

  static void FreeLargeBlock(Byte *buf, uint32_t bufsize);

protected:
  static thread_local LocalMap _localMap;
  // All LocalMaps that the threads are running
  static unordered_set<LocalMap *> _setLocalMap;
  static CachePool *_gCachePool;

  /**The totla memory size that has been allocated.*/
  int64_t _szMemAllocated{0};
  // The actual used bu end user
  int64_t _szMemused{0};
  /**To manage the buffer pools for different size*/
  array<BufferPool *, 28> _arrayPool;
  /**Save the free block memory. It will reused next time.*/
  vector<Buffer *> _vctFreeBuf;
  /**memory cache block used in IResultSet*/
  vector<Byte *> _vctFreeBlock;
  /**Total number allocated result blocks, include free blocks in queue*/
  uint64_t _totalBlockNum;
  /**Mutex for block memory,used to create IDataValue etc. One block can create
   * multi objects.*/
  SpinMutex _spinMutex;

  friend class BufferPool;
  friend class LocalMap;
};
} // namespace storage
