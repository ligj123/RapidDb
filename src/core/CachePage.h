﻿#pragma once
#include "../cache/CachePool.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include "../header.h"
#include "../utils/BytesConvert.h"
#include "../utils/SpinMutex.h"
#include "PageType.h"
#include <atomic>
#include <chrono>

namespace storage {
class IndexTree;

class CachePage {
public:
  static const uint32_t CACHE_PAGE_SIZE;
  static const uint32_t HEAD_PAGE_SIZE;
  static const uint32_t CRC32_PAGE_OFFSET;
  static const uint32_t CRC32_HEAD_OFFSET;

  inline static uint64_t CalcHashCode(uint64_t fileId, uint32_t pageId) {
    return (fileId << 32) + pageId;
  }
  inline static uint64_t GetMsFromEpoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  CachePage(IndexTree *indexTree, PageID pageId, PageType type);
  virtual ~CachePage();
  inline ReentrantSharedSpinMutex &GetLock() { return _rwLock; }
  inline bool IsDirty() const { return _bDirty; }
  inline void SetDirty(bool b) { _bDirty = b; }
  inline bool IsLocked() const { return _rwLock.is_locked(); }
  inline PageID GetPageId() const { return _pageId; }
  inline void UpdateAccessTime() { _dtPageLastAccess = GetMsFromEpoch(); }
  inline uint64_t GetAccessTime() const { return _dtPageLastAccess; }
  inline uint64_t HashCode() const {
    return CalcHashCode(_indexTree->GetFileId(), _pageId);
  }
  inline void UpdateWriteTime() { _dtPageLastWrite = GetMsFromEpoch(); }
  inline uint64_t GetWriteTime() const { return _dtPageLastWrite; }
  inline uint64_t GetFileId() const { return _indexTree->GetFileId(); }
  inline IndexTree *GetIndexTree() const { return _indexTree; }
  inline Byte *GetBysPage() const { return _bysPage; }
  inline PageType GetPageType() const { return _pageType; }

  virtual void ReadPage(PageFile *pageFile);
  virtual void WritePage(PageFile *pageFile);
  virtual bool Releaseable() { return _refCount == 0; }

  inline void ReadLock() { _rwLock.lock_shared(); }
  inline bool ReadTryLock() { return _rwLock.try_lock_shared(); }
  inline void ReadUnlock() { _rwLock.unlock_shared(); }
  inline void WriteLock() { _rwLock.lock(); }
  inline bool WriteTryLock() { return _rwLock.try_lock(); }
  inline void WriteUnlock() { _rwLock.unlock(); }
  int IncRefCount(int num = 1) {
    _refCount.fetch_add(num, memory_order_relaxed);
  }
  int DecRefCount(int num = 1) {
    _refCount.fetch_sub(num, memory_order_relaxed);
  }
  inline int32_t GetRefCount() { return _refCount.load(memory_order_relaxed); }

  inline Byte ReadByte(uint32_t pos) const {
    assert(Configure::GetCachePageSize() > sizeof(Byte) + pos);
    return _bysPage[pos];
  }

  inline void WriteByte(uint32_t pos, Byte value) {
    assert(Configure::GetCachePageSize() > sizeof(Byte) + pos);
    _bysPage[pos] = value;
  }

  inline int16_t ReadShort(uint32_t pos) const {
    assert(Configure::GetCachePageSize() > sizeof(int16_t) + pos);
    return Int16FromBytes(_bysPage + pos);
  }

  inline void WriteShort(uint32_t pos, int16_t value) {
    assert(Configure::GetCachePageSize() > sizeof(int16_t) + pos);
    Int16ToBytes(value, _bysPage + pos);
  }

  inline int32_t ReadInt(uint32_t pos) const {
    assert(Configure::GetCachePageSize() > sizeof(int32_t) + pos);
    return Int32FromBytes(_bysPage + pos);
  }

  inline void WriteInt(uint32_t pos, int32_t value) {
    assert(Configure::GetCachePageSize() > sizeof(int32_t) + pos);
    Int32ToBytes(value, _bysPage + pos);
  }

  uint64_t ReadLong(uint32_t pos) const {
    assert(Configure::GetCachePageSize() > sizeof(int64_t) + pos);
    return Int64FromBytes(_bysPage + pos);
  }

  void WriteLong(uint32_t pos, int64_t value) {
    assert(Configure::GetCachePageSize() > sizeof(int64_t) + pos);
    Int64ToBytes(value, _bysPage + pos);
  }

protected:
  // The page block, it is equal pages in disk
  Byte *_bysPage = nullptr;
  // Read write lock.
  ReentrantSharedSpinMutex _rwLock;
  // Locked when read from or write to disk
  SpinMutex _pageLock;
  // The last time to update _bysPage
  uint64_t _dtPageLastWrite = 0;
  // The last time to visit this page, Now only used when get from
  // PageBufferPool
  uint64_t _dtPageLastAccess = 0;
  // Index Tree
  IndexTree *_indexTree = nullptr;
  // ID for this page
  PageID _pageId = 0;
  // How many times has this page been referenced
  atomic<int32_t> _refCount = 0;
  // If this page has been changed
  bool _bDirty = false;
  // used only in IndexPage, point out if there have records added or deleted
  bool _bRecordUpdate = false;
  // When read this page from disk and verify it by crc32, if return error, set
  // it to true, means this page need to fix.
  bool _bInvalidPage = false;
  // Page Type
  PageType _pageType;
};
} // namespace storage
