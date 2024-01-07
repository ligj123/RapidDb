#pragma once
#include "../cache/CachePool.h"
#include "../config/Configure.h"
#include "../file/PageFile.h"
#include "../header.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorID.h"
#include "../utils/SpinMutex.h"
#include "../utils/ThreadPool.h"
#include "../utils/Utilitys.h"
#include "PageType.h"
#include <atomic>
#include <chrono>
#include <condition_variable>

namespace storage {
class IndexTree;

enum class PageStatus : uint8_t {
  // Not load data from disk
  EMPTY = 0,
  // This page has added reading queue and wait to read
  READING,
  // This page has just finished to read and sync memory
  READED,
  // This page has added writing queue and wait to write
  WRITING,
  // The page has been loaded and the data is valid
  VALID,
  // This page has marked as obsolete and can not be visit again.
  OBSOLETE,
  // The page has been loaded and failed the crc32 verify, need to fix
  INVALID
};

class CachePage {
public:
  static const uint32_t CACHE_PAGE_SIZE;
  static const uint32_t HEAD_PAGE_SIZE;
  static const uint32_t CRC32_PAGE_OFFSET;
  static const uint32_t CRC32_HEAD_OFFSET;

  inline static uint64_t CalcHashCode(uint64_t fileId, uint32_t pageId) {
    return (fileId << 32) + pageId;
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
  // Save contents into page buffer
  virtual bool SaveToBuffer() {
    assert(false);
    return false;
  }
  virtual void AfterRead();
  virtual void AfterWrite() {
    _bDirty = false;
    _pageStatus = PageStatus::VALID;
  }
  virtual void Init() {}
  virtual uint32_t PageSize() const = 0;

  inline bool IsDirty() const { return _bDirty; }
  inline PageID GetPageId() const { return _pageId; }
  inline void UpdateAccessTime() { _dtPageLastAccess = MicroSecTime(); }
  inline DT_MicroSec GetAccessTime() const { return _dtPageLastAccess; }
  inline uint64_t HashCode() const { return CalcHashCode(_fileId, _pageId); }
  inline uint64_t GetFileId() const { return _fileId; }
  inline IndexTree *GetIndexTree() const { return _indexTree; }
  inline Byte *GetBysPage() const { return _bysPage; }
  inline PageType GetPageType() const { return _pageType; }

  inline SharedSpinMutex &GetLock() { return _rwLock; }
  inline bool IsLocked() const { return _rwLock.is_locked(); }
  inline void ReadLock() { _rwLock.lock_shared(); }
  inline bool ReadTryLock() { return _rwLock.try_lock_shared(); }
  inline void ReadUnlock() { _rwLock.unlock_shared(); }
  inline void WriteLock() { _rwLock.lock(); }
  inline bool WriteTryLock() { return _rwLock.try_lock(); }
  inline void WriteUnlock() { _rwLock.unlock(); }

  inline bool IsRefer() { return _bRefer; }
  inline void SetRefer(bool b) { _bRefer = b; }
  virtual bool Releaseable() {
    return !_bRefer && _pageStatus == PageStatus::VALID;
  }

  inline Byte ReadByte(uint32_t pos) const {
    assert(Configure::GetCachePageSize() >= sizeof(Byte) + pos);
    return _bysPage[pos];
  }

  inline void WriteByte(uint32_t pos, Byte value) {
    assert(Configure::GetCachePageSize() >= sizeof(Byte) + pos);
    _bysPage[pos] = value;
  }

  inline int16_t ReadShort(uint32_t pos) const {
    assert(Configure::GetCachePageSize() >= UI16_LEN + pos);
    return Int16FromBytes(_bysPage + pos);
  }

  inline void WriteShort(uint32_t pos, int16_t value) {
    assert(Configure::GetCachePageSize() >= UI16_LEN + pos);
    Int16ToBytes(value, _bysPage + pos);
  }

  inline int32_t ReadInt(uint32_t pos) const {
    assert(Configure::GetCachePageSize() >= UI32_LEN + pos);
    return Int32FromBytes(_bysPage + pos);
  }

  inline void WriteInt(uint32_t pos, int32_t value) {
    assert(Configure::GetCachePageSize() >= UI32_LEN + pos);
    Int32ToBytes(value, _bysPage + pos);
  }

  inline uint64_t ReadLong(uint32_t pos) const {
    assert(Configure::GetCachePageSize() >= UI64_LEN + pos);
    return Int64FromBytes(_bysPage + pos);
  }

  inline void WriteLong(uint32_t pos, int64_t value) {
    assert(Configure::GetCachePageSize() >= UI64_LEN + pos);
    Int64ToBytes(value, _bysPage + pos);
  }

  inline PageStatus GetPageStatus() { return _pageStatus; }
  inline void SetPageStatus(PageStatus s) { _pageStatus = s; }

protected:
  // The page block, it is equal pages in disk
  Byte *_bysPage = nullptr;
  // Read write lock.
  SharedSpinMutex _rwLock;
  // The last time to visit this page, Now only used when get from
  // PageBufferPool
  DT_MicroSec _dtPageLastAccess{0};
  // Index Tree
  IndexTree *_indexTree;
  // ID for this page
  PageID _pageId;
  // Copy from IndexTree's same name variable
  uint32_t _fileId;
  // If this page has been changed
  bool _bDirty{false};
  // used only in IndexPage, point out if there have records added or deleted
  bool _bRecordUpdate{false};
  // Page status, to mark if this page has been loaded and the data is valid.
  PageStatus _pageStatus{PageStatus::EMPTY};
  // Page type
  PageType _pageType;
  // If this page has been refer by index tree or only used in PageBufferPool.
  bool _bRefer{false};
};

} // namespace storage
