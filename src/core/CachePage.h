#pragma once
#include "../cache/CachePool.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include "../file/PageFile.h"
#include "../header.h"
#include "../utils/BytesConvert.h"
#include "../utils/SpinMutex.h"
#include "../utils/ThreadPool.h"
#include "../utils/Utilitys.h"
#include "PageType.h"
#include <atomic>
#include <chrono>

namespace storage {
class IndexTree;

enum class PageStatus : uint8_t {
  // Not load data from disk
  EMPTY = 0,
  // The page has been loaded and the data is valid
  VALID,
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
  void DecRef(int num = 1);
  virtual void ReadPage(PageFile *pageFile = nullptr);
  virtual void WritePage(PageFile *pageFile = nullptr);
  virtual void Init() {}

  inline ReentrantSharedSpinMutex &GetLock() { return _rwLock; }
  inline bool IsDirty() const { return _bDirty; }
  inline void SetDirty(bool b) { _bDirty = b; }
  inline PageID GetPageId() const { return _pageId; }
  inline void UpdateAccessTime() { _dtPageLastAccess = MicroSecTime(); }
  inline uint64_t GetAccessTime() const { return _dtPageLastAccess; }
  inline uint64_t HashCode() const { return CalcHashCode(_fileId, _pageId); }
  inline void UpdateWriteTime() { _dtPageLastWrite = MicroSecTime(); }
  inline uint64_t GetWriteTime() const { return _dtPageLastWrite; }
  inline void UpdateDividTime() { _dtPageLastDivid = MicroSecTime(); }
  inline uint64_t GetDividTime() const { return _dtPageLastDivid; }
  inline uint64_t GetFileId() const { return _fileId; }
  inline IndexTree *GetIndexTree() const { return _indexTree; }
  inline Byte *GetBysPage() const { return _bysPage; }
  inline PageType GetPageType() const { return _pageType; }
  virtual bool Releaseable() { return _refCount == 1; }
  inline bool IsLocked() const { return _rwLock.is_locked(); }
  inline void ReadLock() { _rwLock.lock_shared(); }
  inline bool ReadTryLock() { return _rwLock.try_lock_shared(); }
  inline void ReadUnlock() { _rwLock.unlock_shared(); }
  inline void WriteLock() { _rwLock.lock(); }
  inline bool WriteTryLock() { return _rwLock.try_lock(); }
  inline void WriteUnlock() { _rwLock.unlock(); }
  inline int32_t GetRefCount() { return _refCount.load(memory_order_relaxed); }
  inline bool IsWriteOverTime(uint64_t microSecs) {
    return MicroSecTime() - _dtPageLastWrite > microSecs;
  }
  inline bool IsDividOverTime(uint64_t microSecs) {
    return MicroSecTime() - _dtPageLastDivid > microSecs;
  }
  void IncRef(int num = 1) {
    assert(num > 0);
    _refCount.fetch_add(num, memory_order_relaxed);
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
  inline void SetPageStatus(PageStatus ps) { _pageStatus = ps; }
  inline bool PushWaitTask(Task *task) {
    if (_pageStatus != PageStatus::EMPTY)
      return false;

    unique_lock<SpinMutex> lock(_taskLock);
    if (_pageStatus != PageStatus::EMPTY) {
      return false;
    }
    _waitTasks.push_back(task);

    return true;
  }
  inline MVector<Task *>::Type &GetWaitTasks() { return _waitTasks; }
  inline void SetInDivid(bool b) { _bInDivid.store(b, memory_order_relaxed); }
  inline bool IsInDivid() { return _bInDivid.load(memory_order_relaxed); }
  inline void SetInStorage(bool b) {
    _bInStorage.store(b, memory_order_relaxed);
  }
  inline bool IsInStorage() { return _bInStorage; }
  inline void RemoveTask(Task *task) {
    for (auto iter = _waitTasks.begin(); iter != _waitTasks.end(); iter++) {
      if (*iter == task) {
        _waitTasks.erase(iter);
      }
    }
  }

protected:
  virtual ~CachePage();

protected:
  // To save waiting tasks when reading from disk
  MVector<Task *>::Type _waitTasks;
  // Mutexes when task adding or removing
  SpinMutex _taskLock;
  // The page block, it is equal pages in disk
  Byte *_bysPage = nullptr;
  // Read write lock.
  ReentrantSharedSpinMutex _rwLock;
  // Locked when read from or write to disk
  SpinMutex _pageLock;
  // The last time to divid this page
  DT_MicroSec _dtPageLastDivid = 0;
  // The last time to update _bysPage
  DT_MicroSec _dtPageLastWrite = 0;
  // The last time to visit this page, Now only used when get from
  // PageBufferPool
  DT_MicroSec _dtPageLastAccess = 0;
  // Index Tree
  IndexTree *_indexTree = nullptr;
  // ID for this page
  PageID _pageId = 0;
  // How many times has this page been referenced
  atomic<int32_t> _refCount = {2};
  // Copy from IndexTree's same name variable
  uint32_t _fileId;
  // If this page has been changed
  bool _bDirty = false;
  // used only in IndexPage, point out if there have records added or deleted
  bool _bRecordUpdate = false;
  // Page status, to mark if this page has been loaded and the data is valid.
  PageStatus _pageStatus = PageStatus::EMPTY;
  // Page Type
  PageType _pageType;
  // If this page has been added PageDividPool queue.
  atomic_bool _bInDivid = false;
  // If this page has been added StoragePool queue.
  atomic_bool _bInStorage = false;
};

class ReadPageTask : public Task {
public:
  ReadPageTask(CachePage *page) : _page(page) { page->IncRef(); }
  bool IsSmallTask() { return false; }
  void Run() {
    _status = TaskStatus::RUNNING;
    _page->ReadPage(nullptr);

    if (_page->GetPageStatus() == PageStatus::VALID) {
      _page->Init();
    } else {
      // In following time will add code to fix page;
      abort();
    }

    _status = TaskStatus::STOPED;
    _page->DecRef();
  }

protected:
  CachePage *_page;
};
} // namespace storage
