#pragma once
#include "CachePage.h"
#include "IndexType.h"
#include "RawRecord.h"

namespace storage {
class IndexPage : public CachePage {
public:
  static const float LOAD_FACTOR;
  static const uint16_t PAGE_LEVEL_OFFSET;
  static const uint16_t PAGE_REFERENCE_COUNT;
  static const uint16_t NUM_RECORD_OFFSET;
  static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
  static const uint16_t PARENT_PAGE_POINTER_OFFSET;

public:
  IndexPage(IndexTree* indexTree, uint64_t pageId);
  IndexPage(IndexTree* indexTree, uint64_t pageId, uint8_t pageLevel,
    uint64_t parentPageId);
  ~IndexPage();

  bool PageDivide();
  inline bool IsOverlength() {
    static int32_t max_len = (int32_t)Configure::GetCachePageSize() * 3;
    return _totalDataLength > max_len;
  }
  inline void SetParentPageID(uint64_t parentPageId) {
    _parentPageId = parentPageId;
  }
  inline uint64_t GetParentPageId() { return _parentPageId; }
  inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
  inline bool IsOverTime(uint64_t ms) {
    return GetMsFromEpoch() - GetPageLastUpdateTime() > ms;
  }
  inline uint64_t GetPageLastUpdateTime() { return _dtPageLastUpdate; }
  inline void SetPageLastUpdateTime() { _dtPageLastUpdate = GetMsFromEpoch(); }
  inline uint32_t GetTotalDataLength() { return _totalDataLength; }
  inline uint32_t GetRecordSize() { return _recordNum; }
  inline bool Releaseable() { return _refCount == 0 && _recordRefCount == 0; }
  inline int32_t AddRecordRefCount(int32_t x) {
    _recordRefCount += x;
    return _recordRefCount;
  }
  inline int32_t GetRecordRefCount() { return _recordRefCount; }
  inline int32_t GetRecordNum() { return _recordNum; }
  inline bool TryUpdateLock() { return _updateLock.try_lock(); }
  inline void UpdateLock() { _updateLock.lock(); }
  inline void UpdateUnlock() { _updateLock.unlock(); }

  virtual void Init();
  virtual uint16_t GetMaxDataLength() const = 0;
  virtual bool SaveRecords() = 0;

protected:
  utils::SpinMutex _updateLock;
  vector<RawRecord*> _vctRecord;
  uint64_t _parentPageId = 0;
  uint64_t _dtPageLastUpdate = 0;
  int32_t _totalDataLength = 0;
  int32_t _recordNum = 0;
  int32_t _recordRefCount = 0;
};
} // namespace storage
