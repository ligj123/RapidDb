#pragma once
#include "../cache/CachePool.h"
#include "../config/Configure.h"
#include "../header.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorID.h"
#include "../utils/SpinMutex.h"
#include "../utils/ThreadPool.h"
#include "../utils/Utilitys.h"
#include "CoreEnum.h"
#include <atomic>
#include <chrono>
#include <condition_variable>

namespace storage {
class IndexTree;

class CachePage {
public:
  // Index page size
  static const uint32_t INDEX_PAGE_SIZE;
  // Head page size
  static const uint32_t HEAD_PAGE_SIZE;
  // Offset to wrtie crc32 in index page
  static const uint32_t CRC32_INDEX_OFFSET;
  // Offset to write crc32 in head page
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
  virtual ~CachePage() {}
  void SaveCrc32() { assert(false); }
  // Called after async read.
  virtual void AfterRead() = 0;
  // Called after async write.
  virtual void AfterWrite() {
    _pageStatus.store(PageStatus::VALID, memory_order_relaxed);
  }
  // Initialize page parameters from page buffer after read page from disk.
  virtual void InitParameters() {}
  virtual uint32_t PageSize() const = 0;
  // Get this page position in index file
  uint64_t FileOffset() const {
    if (_pageType == PageType::HEAD_PAGE) {
      return 0;
    } else {
      return HEAD_PAGE_SIZE + (uint64_t)INDEX_PAGE_SIZE * _pageId;
    }
  }
  // If need save the page into CachePagePool, implement this method in child
  // class. It will calculate the score to decide to release which pages.
  uint32_t CalcScore() {
    if (_bRefered) {
      if (_score < 30000)
        _score *= 1.5;
    } else {
      if (_score > 5) {
        _score *= 0.66;
      }
    }
    return _score;
  }

  inline bool IsDirty() const { return _bDirty; }
  inline void SetDirty(bool b = true) { _bDirty = b; }
  inline PageID GetPageId() const { return _pageId; }
  inline uint64_t HashCode() const { return CalcHashCode(_fileId, _pageId); }
  inline uint64_t GetFileId() const { return _fileId; }
  inline IndexTree *GetIndexTree() const { return _indexTree; }
  inline Byte *GetBysPage() const { return _bysPage; }
  inline PageType GetPageType() const { return _pageType; }

  inline SpinMutex &GetLock() { return _spinLock; }
  inline bool IsLocked() const { return _spinLock.is_locked(); }
  inline void Lock() { _spinLock.lock(); }
  inline bool TryLock() { return _spinLock.try_lock(); }
  inline void Unlock() { _spinLock.unlock(); }

  virtual bool Releaseable() {
    return !_bRefered &&
           _pageStatus.load(memory_order_relaxed) == PageStatus::VALID;
  }
  virtual Byte GetPageLevel() { return UINT8_MAX; }

  inline Byte ReadByte(uint32_t pos) const { return _bysPage[pos]; }

  inline void WriteByte(uint32_t pos, Byte value) { _bysPage[pos] = value; }

  inline int16_t ReadShort(uint32_t pos) const {
    return Int16FromBytes(_bysPage + pos);
  }

  inline void WriteShort(uint32_t pos, int16_t value) {
    Int16ToBytes(value, _bysPage + pos);
  }

  inline int32_t ReadInt(uint32_t pos) const {
    return Int32FromBytes(_bysPage + pos);
  }

  inline void WriteInt(uint32_t pos, int32_t value) {
    Int32ToBytes(value, _bysPage + pos);
  }

  inline uint64_t ReadLong(uint32_t pos) const {
    return Int64FromBytes(_bysPage + pos);
  }

  inline void WriteLong(uint32_t pos, int64_t value) {
    Int64ToBytes(value, _bysPage + pos);
  }

  inline PageStatus GetPageStatus() {
    return _pageStatus.load(memory_order_relaxed);
  }
  inline void SetPageStatus(PageStatus s, bool acquire = false) {
    _pageStatus.store(s, memory_order_relaxed);
    if (acquire) {
      _pageStatus.load(memory_order_acquire);
    }
  }
  inline uint32_t AddWaiting(uint32_t num) {
    _waiting += num;
    return _waiting;
  }
  inline uint32_t GetWaiting() { return _waiting; }

  inline void AddWriteQueue(MTreeMap<uint64_t, CachePage *> &pageMap) {
    if (_bWriteQueue)
      return;

    pageMap.emplace(((uint64_t)GetPageLevel() << 32) + GetPageId(), this);
    _bWriteQueue = true;
  }

  inline void ClearWriteQueue() { _bWriteQueue = false; }
  void SetNeedDisk(bool b) { _bNeedDisk = true; }
  bool IsNeedDisk() { return _bNeedDisk; }

protected:
  // The page byte array to save contents. It should be assigned and released
  // inchild class.
  Byte *_bysPage = nullptr;
  // Spin lock.
  SpinMutex _spinLock;
  // Index Tree
  IndexTree *_indexTree;
  // ID for this page
  PageID _pageId;
  // Copy from IndexTree's same name variable
  uint32_t _fileId;
  // If this page has been changed
  bool _bDirty{false};
  // The records have changed or not, only used for IndexPage
  bool _bRecordUpdated{false};
  // Page status, to mark if this page has been loaded and the data is valid.
  atomic<PageStatus> _pageStatus{PageStatus::EMPTY};
  // Page type
  PageType _pageType;
  // True: This page has been referred by IndexTask and can not be freed.
  bool _bRefered{true};
  // If it has added into write queue
  bool _bWriteQueue{false};
  // If the page block has been updated and need to write page into disk
  bool _bNeedDisk{false};
  // Every page has a score. it will be calculated according to page type,
  // previous score, the visit times in current period. CachePagePool will clear
  // pages in cycle according score.
  uint32_t _score{100000};
  // How many statements are waiting for this page, if > 0, this page can not be
  // free.
  uint32_t _waiting{0};
};

struct CachePageHash {
  size_t operator()(const CachePage *page) const { return page->HashCode(); }
};

struct CachePageEqual {
  bool operator()(const CachePage *lpage, const CachePage *rpage) const {
    return lpage->HashCode() == rpage->HashCode();
  }
};

struct CachePageLess {
  bool operator()(const CachePage *lpage, const CachePage *rpage) const {
    return lpage->HashCode() < rpage->HashCode();
  }
};

using HashSetPage = MHashSet<CachePage *, CachePageHash, CachePageEqual>;
using TreeSetPage = MTreeSet<CachePage *, CachePageLess>;
} // namespace storage
