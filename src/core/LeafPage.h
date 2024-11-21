#pragma once
#include "../utils/ErrorMsg.h"
#include "HeadPage.h"
#include "IndexPage.h"
#include "RawKey.h"

namespace storage {
class LeafRecord;

class LeafPage : public IndexPage {
public:
  static const uint16_t PREV_PAGE_POINTER_OFFSET;
  static const uint16_t NEXT_PAGE_POINTER_OFFSET;
  static const uint16_t DATA_BEGIN_OFFSET;
  static void RollbackLeafRecords(const MVector<LeafRecord *> &vctRec,
                                  int64_t endPos);

public:
  // Create a new leaf page
  LeafPage(IndexTree *indexTree, PageID pageId, PageID parentPageId)
      : IndexPage(indexTree, pageId, 0, parentPageId, PageType::LEAF_PAGE) {}

  // Create for existed page
  LeafPage(IndexTree *indexTree, PageID pageId)
      : IndexPage(indexTree, pageId, PageType::LEAF_PAGE) {}
  ~LeafPage();
  void InitParameters() override;

  inline void SetPrevPageId(PageID id) {
    _prevPageId = id;
    _bDirty = true;
  }
  inline PageID GetPrevPageId() { return _prevPageId; }
  inline void SetNextPageId(PageID id) {
    _nextPageId = id;
    _bDirty = true;
  }
  inline PageID GetNextPageId() { return _nextPageId; }

  inline void SetPrevPage(LeafPage *page) { _prevPage = page; }
  inline LeafPage *GetPrevPage() { return _prevPage; }
  inline void SetNextPage(LeafPage *page) { _nextPage = page; }
  inline LeafPage *GetNextPage() { return _nextPage; }

  bool IsOverlength() override {
    return _committedDataLength >= MAX_DATA_LENGTH_LEAF;
  }
  bool Releaseable() override {
    return !_bRefered &&
           _pageStatus.load(memory_order_relaxed) == PageStatus::VALID;
  }
  /**
   * @brief Insert a leaf record into position pos in this page
   * @param lr The leaf record will be inserted
   * @param pos The position for insert.
   */
  void InsertRecord(LeafRecord *lr, int32_t pos);
  /**
   * @brief For test aim, if insert fail will put the error message into
   * _threadErrorMsg
   * @param lr The leaf record will be inserted
   * @return True: succeed to insert the record; False: failed to insert and set
   * the failed reason into ErrorMsg::_threadErrorMsg
   */
  bool InsertRecord(LeafRecord *lr) {
    bool bFind;
    int32_t pos = SearchRecord(*lr, bFind);
    if (bFind) {
      _threadErrorMsg.reset(new ErrorMsg(CORE_REPEATED_RECORD, {}));
      return false;
    }

    InsertRecord(lr, pos);
    return true;
  }
  /** @brief Add a new record to the last position of this page. Only used wehn
   * batch add for ordered records, does not need transaction.
   * @param record The new record
   * @return True: passed to add the record; False: failed to add the record due
   * to reach length limit.
   */
  bool AddRecord(LeafRecord *record);
  /**
   * @brief Save the records content into byte array
   * @param pageSet If there has OverflowPages that need to write disk, add into
   * this set
   * @param block If there have multi thread tasks for this index set it to true
   * @return Success to save or not
   */
  bool SaveRecords(MTreeMap<uint64_t, CachePage *> &pageMap, bool block);
  /**`
   * @brief Get the Record in this LeafPage with position=pos
   * @param pos The position of records in this page
   * @return LeafRecord The leaf record to get
   */
  LeafRecord &GetRecord(int32_t pos);
  int32_t SearchRecord(const LeafRecord &rr, bool &bFind, int32_t start = 0,
                       int32_t end = INT32_MAX);
  int32_t SearchKey(const RawKey &key, bool &bFind, int32_t start = 0,
                    int32_t end = INT32_MAX);
  int32_t SearchKey(const LeafRecord &rr, bool &bFind, int32_t start = 0,
                    int32_t end = INT32_MAX);

  void ClearRecords();
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords();
  bool SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap,
                 Byte lockPageLevel = UINT8_MAX) override;

protected:
  int CompareTo(uint32_t recPos, const RawKey &key);
  int CompareTo(uint32_t recPos, const LeafRecord &rr, bool key);

protected:
  uint32_t _prevPageId{PAGE_NULL_POINTER};
  uint32_t _nextPageId{PAGE_NULL_POINTER};
  LeafPage *_prevPage{nullptr};
  LeafPage *_nextPage{nullptr};
  bool _bRangeEndPage{false};
  bool _bRangeBeginPage{false};
};
} // namespace storage
