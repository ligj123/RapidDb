#pragma once
#include "../cache/CachePool.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include "../header.h"
#include "../utils/BytesConvert.h"
#include "../utils/SpinMutex.h"
#include <atomic>
#include <chrono>

namespace storage {
class IndexTree;

class CachePage {
public:
  static const uint32_t CRC32_INDEX_OFFSET;
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
  CachePage(IndexTree *indexTree, PageID pageId);
  virtual ~CachePage();
  bool IsFileClosed() const;

  inline ReentrantSharedSpinMutex &GetLock() { return _rwLock; }
  inline bool IsDirty() const { return _bDirty; }
  inline void SetDirty(bool b) { _bDirty = b; }
  inline bool IsLocked() const { return _rwLock.is_locked(); }
  inline PageID GetPageId() const { return _pageId; }
  inline void UpdateAccessTime() { _dtPageLastAccess = GetMsFromEpoch(); }
  inline uint64_t GetAccessTime() const { return _dtPageLastAccess; }
  inline uint64_t HashCode() const { return CalcHashCode(_fileId, _pageId); }
  inline void UpdateWriteTime() { _dtPageLastWrite = GetMsFromEpoch(); }
  inline uint64_t GetWriteTime() const { return _dtPageLastWrite; }
  inline uint64_t GetFileId() const { return _fileId; }
  inline IndexTree *GetIndexTree() const { return _indexTree; }
  inline Byte *GetBysPage() const { return _bysPage; }
  virtual void ReadPage();
  virtual void WritePage();
  virtual bool Releaseable() { return _refCount == 0; }

  inline void ReadLock() { _rwLock.lock_shared(); }
  inline bool ReadTryLock() { return _rwLock.try_lock_shared(); }
  inline void ReadUnlock() { _rwLock.unlock_shared(); }
  inline void WriteLock() { _rwLock.lock(); }
  inline bool WriteTryLock() { return _rwLock.try_lock(); }
  inline void WriteUnlock() { _rwLock.unlock(); }
  void IncRefCount();
  void DecRefCount();
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
  Byte *_bysPage = nullptr;
  ReentrantSharedSpinMutex _rwLock;
  SpinMutex _pageLock;
  uint64_t _dtPageLastWrite = 0;
  uint64_t _dtPageLastAccess = 0;
  IndexTree *_indexTree = nullptr;

  PageID _pageId = 0;
  uint16_t _fileId = 0;
  atomic<int32_t> _refCount = 0;
  bool _bDirty = false;
  bool _bRecordUpdate = false;
  bool _bLastPage = false;
};
} // namespace storage
