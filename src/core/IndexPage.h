#pragma once
#include "../cache/Mallocator.h"
#include "CachePage.h"
#include "IndexType.h"
#include "RawRecord.h"

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
  // For existed page and will put it to read queue
  IndexPage(IndexTree *indexTree, uint32_t pageId, PageType type);
  // To create a new index page
  IndexPage(IndexTree *indexTree, uint32_t pageId, uint8_t pageLevel,
            uint32_t parentPageId, PageType type);
  ~IndexPage();
  void LoadVars() override;
  inline uint16_t GetMaxDataLength() const {
    return _pageType == PageType::LEAF_PAGE ? MAX_DATA_LENGTH_LEAF
                                            : MAX_DATA_LENGTH_BRANCH;
  };
  // Save records from vector and other variable into buffer
  virtual bool SaveRecords() = 0;
  // Split current page if this page's length exceed LOAD_FACTOR
  virtual bool SplitPage(bool block = false) = 0;

  inline bool IsOverLoadThreshold() {
    return _totalDataLength > LOAD_THRESHOLD;
  }
  inline void SetParentPageID(PageID parentPageId) {
    _parentPageId = parentPageId;
  }
  inline PageID GetParentPageId() { return _parentPageId; }
  inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
  inline uint32_t GetTotalDataLength() { return _totalDataLength; }
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
  // Get the number of records in transaction status, only valid for leaf page
  inline uint32_t GetTranCount() { return _tranCount; }
  inline void SetParentPage(IndexPage *parentPage) { _parentPage = parentPage; }
  inline IndexPage *GetParentPage() { return _parentPage; }

protected:
  // Parent page ID
  uint32_t _parentPageId{0};
  // Total data length in this page
  uint32_t _totalDataLength{0};
  // The record number in this page
  uint32_t _recordNum{0};
  // How many records are in transaction status, only used in LeafPage
  uint32_t _tranCount{0};
  // parent page
  IndexPage *_parentPage{nullptr};
};
} // namespace storage
