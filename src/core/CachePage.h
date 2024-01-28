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
  // This page has just finished to read and need to sync memory
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
  virtual ~CachePage();
  // Save contents into page buffer
  virtual bool SaveToBuffer() {
    assert(false);
    return false;
  }
  void SaveCrc();
  virtual void AfterRead();
  virtual void AfterWrite() { _pageStatus = PageStatus::VALID; }
  // Load page variable from page buffer after read page from disk.
  virtual void LoadVars() {}
  virtual uint32_t PageSize() const = 0;
  uint64_t FileOffset() const {
    if (_pageType == PageType::HEAD_PAGE) {
      return 0;
    } else {
      return HEAD_PAGE_SIZE + (uint64_t)INDEX_PAGE_SIZE * _pageId;
    }
  }
  virtual void CalcScore() { assert(false); }
  inline uint32_t GetScore() const { return _score; }

  inline bool IsDirty() const { return _bDirty; }
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

  inline uint32_t IsRefered() { return _bRefered; }
  inline void SetReferred(bool b) { _bRefered = b; }
  virtual bool Releaseable() {
    return !_bRefered && (_pageStatus != PageStatus::READING ||
                          _pageStatus != PageStatus::WRITING);
  }

  inline Byte ReadByte(uint32_t pos) const {
    assert(Configure::GetIndexPageSize() >= sizeof(Byte) + pos);
    return _bysPage[pos];
  }

  inline void WriteByte(uint32_t pos, Byte value) {
    assert(Configure::GetIndexPageSize() >= sizeof(Byte) + pos);
    _bysPage[pos] = value;
  }

  inline int16_t ReadShort(uint32_t pos) const {
    assert(Configure::GetIndexPageSize() >= UI16_LEN + pos);
    return Int16FromBytes(_bysPage + pos);
  }

  inline void WriteShort(uint32_t pos, int16_t value) {
    assert(Configure::GetIndexPageSize() >= UI16_LEN + pos);
    Int16ToBytes(value, _bysPage + pos);
  }

  inline int32_t ReadInt(uint32_t pos) const {
    assert(Configure::GetIndexPageSize() >= UI32_LEN + pos);
    return Int32FromBytes(_bysPage + pos);
  }

  inline void WriteInt(uint32_t pos, int32_t value) {
    assert(Configure::GetIndexPageSize() >= UI32_LEN + pos);
    Int32ToBytes(value, _bysPage + pos);
  }

  inline uint64_t ReadLong(uint32_t pos) const {
    assert(Configure::GetIndexPageSize() >= UI64_LEN + pos);
    return Int64FromBytes(_bysPage + pos);
  }

  inline void WriteLong(uint32_t pos, int64_t value) {
    assert(Configure::GetIndexPageSize() >= UI64_LEN + pos);
    Int64ToBytes(value, _bysPage + pos);
  }

  inline PageStatus GetPageStatus() { return _pageStatus; }
  inline void SetPageStatus(PageStatus s) { _pageStatus = s; }
  inline uint32_t AddWaiting(uint32_t num) {
    _waiting += num;
    return _waiting;
  }
  inline uint32_t GetWaiting() { return _waiting; }

protected:
  // The page block, it is equal pages in disk
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
  // Page status, to mark if this page has been loaded and the data is valid.
  PageStatus _pageStatus{PageStatus::EMPTY};
  // Page type
  PageType _pageType;
  // used only in IndexPage, point out if there have records added or deleted
  bool _bRecordUpdate{false};
  // True: This page has been referred by IndexTask and can not be freed.
  bool _bRefered{true};
  // In current period this page has been visit how many time. It will be used
  // to calc score and will clear to zero after calc score.
  uint16_t _visit{0};
  // Every time to scan the CachePagePool, it will calc the new score for every
  // page according to last score and if it has been referred.
  uint32_t _score{100000};
  // How many statements are waiting for this page.
  uint32_t _waiting{0};
};

} // namespace storage
