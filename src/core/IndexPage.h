#pragma once
#include "../cache/Mallocator.h"
#include "CachePage.h"
#include "IndexType.h"
#include "RawRecord.h"

namespace storage {
class AbsoleteBuffer {
public:
  AbsoleteBuffer(Byte *bys, int refCount) : _bys(bys), _refCount(refCount) {}
  void ReleaseCount(int num) {
    int val = _refCount.fetch_sub(num);
    assert(val >= num);
    if (val == num) {
      CachePool::ReleasePage(_bys);
      delete this;
    }
  }

  bool IsDiffBuff(Byte *buf) const { return (buf != _bys); }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  Byte *_bys;
  std::atomic<int> _refCount;
};

class IndexPage : public CachePage {
public:
  // Percentage for a page to used. if surpass, will split the following records
  // into next page
  static const uint16_t LOAD_FACTOR;
  // The max length for a page. If surpass it, will divide this page at once.
  static const uint32_t LOAD_THRESHOLD;
  // Page level, leaf page=0, branch page from 1 start
  static const uint16_t PAGE_LEVEL_OFFSET;
  // To save if this page is the last page in current level. The highest bit is
  // for begin page mark, the second bit is for end page mark.
  static const uint16_t PAGE_BEGIN_END_OFFSET;
  // Used in future. For large transaction, to save how much record in
  // transaction status, only used in LeafPage.
  static const uint16_t PAGE_TRAN_COUNT;
  // Recors number in this page
  static const uint16_t NUM_RECORD_OFFSET;
  // Total data length in this page
  static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
  // Parent page point
  static const uint16_t PARENT_PAGE_POINTER_OFFSET;
  // The max records length in leaf page
  static const uint16_t MAX_DATA_LENGTH_LEAF;
  // The max records length in branch page
  static const uint16_t MAX_DATA_LENGTH_BRANCH;

public:
  IndexPage(IndexTree *indexTree, uint32_t pageId, PageType type);
  IndexPage(IndexTree *indexTree, uint32_t pageId, uint8_t pageLevel,
            uint32_t parentPageId, PageType type);
  ~IndexPage();
  virtual void Init();
  inline uint16_t GetMaxDataLength() const {
    return GetPageType() == PageType::LEAF_PAGE ? MAX_DATA_LENGTH_LEAF
                                                : MAX_DATA_LENGTH_BRANCH;
  };
  virtual bool SaveRecords() = 0;
  bool PageDivide();

  inline bool IsOverlength() { return _totalDataLength > LOAD_THRESHOLD; }
  inline void SetParentPageID(PageID parentPageId) {
    _parentPageId = parentPageId;
  }
  inline PageID GetParentPageId() { return _parentPageId; }
  inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
  inline uint32_t GetTotalDataLength() { return _totalDataLength; }
  inline uint32_t GetRecordNumber() { return _recordNum; }
  bool Releaseable() override { return _refCount == 1 && _tranCount == 0; }
  inline bool IsBeginPage() { return _bysPage[PAGE_BEGIN_END_OFFSET] & 0x80; }
  inline void SetBeginPage(bool bBegin) {
    _bysPage[PAGE_BEGIN_END_OFFSET] =
        bBegin ? (_bysPage[PAGE_BEGIN_END_OFFSET] | 0x80)
               : (_bysPage[PAGE_BEGIN_END_OFFSET] & 0x7f);
  }
  inline bool IsEndPage() { return _bysPage[PAGE_BEGIN_END_OFFSET] & 0x40; }
  inline void SetEndPage(bool bEnd) {
    _bysPage[PAGE_BEGIN_END_OFFSET] =
        bEnd ? (_bysPage[PAGE_BEGIN_END_OFFSET] | 0x40)
             : (_bysPage[PAGE_BEGIN_END_OFFSET] & 0xbf);
  }
  inline uint32_t GetTranCount() { return _tranCount; }

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
  uint32_t _totalDataLength = 0;
  // The record number in this page
  uint32_t _recordNum = 0;
  // How many records are in transaction status
  uint32_t _tranCount = 0;
};
} // namespace storage
