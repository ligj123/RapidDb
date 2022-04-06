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
  // Page level, leaf page=0, branch page from 1 start
  static const uint16_t PAGE_LEVEL_OFFSET;
  // If this page the last page in current level
  static const uint16_t PAGE_LAST_OFFSET;
  // Used in future
  static const uint16_t PAGE_TRAN_COUNT;
  // Recors number in this page
  static const uint16_t NUM_RECORD_OFFSET;
  // Total data length in this page
  static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
  // Parent page point
  static const uint16_t PARENT_PAGE_POINTER_OFFSET;

public:
  IndexPage(IndexTree *indexTree, uint32_t pageId, PageType type);
  IndexPage(IndexTree *indexTree, uint32_t pageId, uint8_t pageLevel,
            uint32_t parentPageId, PageType type);
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
  inline uint32_t GetTotalDataLength() { return _totalDataLength; }
  inline uint32_t GetRecordSize() { return _recordNum; }
  bool Releaseable() override { return _refCount == 0; }
  inline uint32_t GetRecordNum() { return _recordNum; }
  inline bool IsLastPage() { return _bysPage[PAGE_LAST_OFFSET] != 0; }
  inline void SetLastPage(bool last) {
    _bysPage[PAGE_LAST_OFFSET] = (last ? 1 : 0);
  }

  virtual void Init();
  virtual uint16_t GetMaxDataLength() const = 0;
  virtual bool SaveRecords() = 0;

protected:
  // When split this page into several pages, the records saved in _bysPage will
  // copy into new pages when new page call WritePage. So only after all related
  // pages has been saved, the old _bysPage can be released.
  AbsoleteBuffer *_absoBuf = nullptr;
  // The vector to save records in this page
  MVector<RawRecord *>::Type _vctRecord;
  // Parent page ID
  uint32_t _parentPageId = 0;
  // Total data length in this page
  int32_t _totalDataLength = 0;
  // The record number in this page
  int32_t _recordNum = 0;
};
} // namespace storage
