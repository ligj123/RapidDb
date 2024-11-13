#pragma once
#include "../cache/Mallocator.h"
#include "CachePage.h"
#include "CoreEnum.h"
#include "RawRecord.h"

#include <boost/crc.hpp>

#define BEGIN_PAGE_BIT 0x80
#define END_PAGE_BIT 0x40
#define NOT_BEGIN_PAGE_BIT 0x7F
#define NOT_END_PAGE_BIT 0xBF

namespace storage {
class HeadPage;

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
  // Used in future. For large transaction, to save how much records in
  // transaction status, only used in LeafPage.
  static const uint16_t PAGE_TRAN_COUNT_OFFSET;
  // Records number in this page
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
  // To construct an existed page and it need to put it into read queue
  IndexPage(IndexTree *indexTree, uint32_t pageId, PageType type)
      : CachePage(indexTree, pageId, type) {
    _bysPage = CachePool::ApplyPage();
  }
  // To create a new index page
  IndexPage(IndexTree *indexTree, uint32_t pageId, uint8_t pageLevel,
            uint32_t parentPageId, PageType type)
      : CachePage(indexTree, pageId, type) {
    _bysPage = CachePool::ApplyPage();
    _bysPage[PAGE_LEVEL_OFFSET] = (Byte)pageLevel;
    _parentPageId = parentPageId;
    // New page, do not read data from disk and init.
    _pageStatus.store(PageStatus::VALID, memory_order_relaxed);
  }
  ~IndexPage() override;

  inline uint16_t GetMaxDataLength() const {
    return _pageType == PageType::LEAF_PAGE ? MAX_DATA_LENGTH_LEAF
                                            : MAX_DATA_LENGTH_BRANCH;
  };

  inline void SetParentPageID(PageID parentPageId) {
    _parentPageId = parentPageId;
    _bDirty = true;
  }
  void AfterRead() override {
    boost::crc_32_type crc32;
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_INDEX_OFFSET);
    if (crc32.checksum() != (uint32_t)ReadInt(CRC32_INDEX_OFFSET)) {
      _pageStatus.store(PageStatus::INVALID, memory_order_relaxed);
      // TO DO
      // Now if cache page is invalid, it will abort; In following version, it
      // will add the function to fix the invalid page
      abort();
    } else {
      _bDirty = false;
      InitParameters();
      if (_parentPage != nullptr && _parentPage->GetPageId() != _parentPageId)
          [[unlikely]] {
        _parentPageId = _parentPage->GetPageId();
        _bDirty = true;
        _pageStatus.store(PageStatus::READED, memory_order_release);
      }
    }
  }
  inline PageID GetParentPageId() { return _parentPageId; }
  inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
  inline uint32_t GetCommitedDataLength() { return _committedDataLength; }
  inline uint32_t GetTempDataLength() {
    assert(_pageType == PageType::LEAF_PAGE);
    return _tempDataLength;
  }
  inline uint32_t GetRecordNumber() { return _recordNum; }
  inline bool IsBeginPage() {
    return _bysPage[PAGE_BEGIN_END_OFFSET] & BEGIN_PAGE_BIT;
  }
  inline void SetBeginPage(bool bBegin) {
    _bysPage[PAGE_BEGIN_END_OFFSET] =
        bBegin ? (_bysPage[PAGE_BEGIN_END_OFFSET] | BEGIN_PAGE_BIT)
               : (_bysPage[PAGE_BEGIN_END_OFFSET] & NOT_BEGIN_PAGE_BIT);
  }
  inline bool IsEndPage() {
    return _bysPage[PAGE_BEGIN_END_OFFSET] & END_PAGE_BIT;
  }
  inline void SetEndPage(bool bEnd) {
    _bysPage[PAGE_BEGIN_END_OFFSET] =
        bEnd ? (_bysPage[PAGE_BEGIN_END_OFFSET] | END_PAGE_BIT)
             : (_bysPage[PAGE_BEGIN_END_OFFSET] & NOT_END_PAGE_BIT);
  }
  inline void SetParentPage(IndexPage *parentPage) { _parentPage = parentPage; }
  inline IndexPage *GetParentPage() { return _parentPage; }
  uint32_t PageSize() const override { return INDEX_PAGE_SIZE; }

  virtual bool IsOverlength() = 0;

  /**
   * @brief Split current page if this page's length exceed LOAD_FACTOR
   * @param pageSet The save the changed pages and put them into write queue in
   * future
   * @param pageLevel The page level that the BranchRecords in those pages will
   * split into multi index tasks to run the statement.
   *                  If =0xFF, means only one index task to run.
   * @return True: The split conditions can be meet and has split this page
   *         False: Failed to split the page
   */
  virtual bool SplitPage(MHashSet<CachePage *> &pageSet,
                         Byte pageLevel = 0xFF) = 0;

protected:
  // Parent page ID
  uint32_t _parentPageId{0};
  // Total commited data length in this page
  uint32_t _committedDataLength{0};
  // Total data length in the page, include Committed and uncommitted records,
  // only used for LeafPage.
  uint32_t _tempDataLength{0};
  // The record number in this page
  uint32_t _recordNum{0};
  // parent page
  IndexPage *_parentPage{nullptr};
  // The vector to save records in this page
  MVector<RawRecord *> _vctRecord;
};
} // namespace storage
