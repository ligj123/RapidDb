#pragma once
#include "../cache/AbsoleteBuffer.h"
#include "../cache/Mallocator.h"
#include "CachePage.h"
#include "IndexType.h"
#include "RawRecord.h"

namespace storage {
class IndexPage : public CachePage {
public:
  static const float LOAD_FACTOR;
  static const uint16_t PAGE_TYPE_OFFSET;
  static const uint16_t PAGE_LEVEL_OFFSET;
  static const uint16_t PAGE_REFERENCE_COUNT;
  static const uint16_t NUM_RECORD_OFFSET;
  static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
  static const uint16_t PARENT_PAGE_POINTER_OFFSET;

public:
  IndexPage(IndexTree *indexTree, uint32_t pageId);
  IndexPage(IndexTree *indexTree, uint32_t pageId, uint8_t pageLevel,
            uint32_t parentPageId);
  ~IndexPage();

  bool PageDivide();
  inline bool IsOverlength() {
    return _totalDataLength > Configure::GetCachePageSize() * 3;
  }
  inline void SetParentPageID(uint32_t parentPageId) {
    _parentPageId = parentPageId;
  }
  inline uint32_t GetParentPageId() { return _parentPageId; }
  inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
  inline bool IsOverTime(uint64_t ms) {
    return GetMsFromEpoch() - GetPageLastUpdateTime() > ms;
  }
  inline uint64_t GetPageLastUpdateTime() { return _dtPageLastUpdate; }
  inline void SetPageLastUpdateTime() { _dtPageLastUpdate = GetMsFromEpoch(); }
  inline uint32_t GetTotalDataLength() { return _totalDataLength; }
  inline uint32_t GetRecordSize() { return _recordNum; }
  bool Releaseable() override { return _refCount == 0 && _tranCount == 0; }
  inline uint32_t AddRecordTranCount(uint32_t x) {
    _tranCount += x;
    return _tranCount;
  }
  inline uint32_t GetRecordTranCount() { return _tranCount; }
  inline uint32_t GetRecordNum() { return _recordNum; }

  virtual void Init();
  virtual uint16_t GetMaxDataLength() const = 0;
  virtual bool SaveRecords() = 0;

protected:
  AbsoleteBuffer *_absoBuf = nullptr;
  MVector<RawRecord *>::Type _vctRecord;
  uint64_t _dtPageLastUpdate = 0;
  uint32_t _parentPageId = 0;
  int32_t _totalDataLength = 0;
  int32_t _recordNum = 0;
  int32_t _tranCount = 0;
};
} // namespace storage
