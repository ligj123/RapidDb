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
  inline void SetNextPage(LeafPage *page) { _nextPage = page; }

  bool IsOverlength() override {
    return _committedDataLength >= MAX_DATA_LENGTH_LEAF ||
           _tempDataLength >= MAX_DATA_LENGTH_LEAF;
  }
  bool Releaseable() override {
    return !_bRefered &&
           _pageStatus.load(memory_order_relaxed) == PageStatus::VALID;
  }

  /**
   * @brief Get the previoue page. This method is unused for a while
   * @param bLoad If the previous page is not in memory, load it from disk or
   * not
   * @return The previous page
   */
  LeafPage *GetPrevPage(bool bLoad = true);
  /**
   * @brief Get the next page
   * @param bLoad If the next page is not in memory, load it from disk or not
   * @return The next page
   */
  LeafPage *GetNextPage(bool bLoad = true);

  /**
   * @brief Insert a leaf record into position pos in this page
   * @param lr The leaf record will be inserted
   * @param pos The position for insert.
   */
  void InsertRecord(LeafRecord *lr, int32_t pos);

  /**
   * @brief Insert or delete a LeafRecord
   */
  void UpdateAction(LeafRecord *lr);
  /**
   * @brief Delete a LeafRecord, only use new LeafRecord with delete status to
   * replace old record, old record will save into _undoRec and all of them will
   * be removed when commit or recover old record when rollback.
   * @param lr New LeafRecor with delete status
   * @param pos The position of record to delete
   */
  void DeleteRecord(LeafRecord *lr, int32_t pos);
  /** @brief Add a new record to the last position of this page. Only used wehn
   * batch add for ordered records, does not need transaction.
   * @param record The new record
   * @param bFullPage True: Add the records into page to full all able spaces;
   *                  False: Not exceed the spaces of LOAD_FACTOR.
   * @return True: passed to add the record; False: failed to add the record due
   * to reach length limit.
   */
  bool AppendRecord(LeafRecord *record, bool bFullPage = true);
  /**
   * @brief Save the records content into byte array
   * @param pageSet If there has OverflowPages that need to write disk, add into
   * this set
   * @return True: The page is clean and all data has been saved into buffer;
   * False: The page has dirty data and can not save all of data into buffer.
   */
  bool SaveRecords(MTreeMap<uint64_t, CachePage *> &pageMap);
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

  void ClearRecords() override;
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords() override;
  bool SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap) override;
  /**
   * @brief Clear all RecordLocks in this page if the locks has been commited or
   * rollbacked.
   */
  void ClearObsoleteLocks();

  ReleaseResult ReleaseLock(LeafRecord *lr);

  inline bool IsRangBeginPage() { return _bRangeBeginPage; }
  inline void SetRangeBeginPage(bool b) { _bRangeBeginPage = b; }
  inline bool IsRangEndPage() { return _bRangeEndPage; }
  inline void SetRangeEndPage(bool b) { _bRangeEndPage = b; }
  inline void SetRecordUpdated() {
    _bDirty = true;
    _bRecordUpdated = true;
  }

protected:
  uint32_t _prevPageId{PAGE_NULL_POINTER};
  uint32_t _nextPageId{PAGE_NULL_POINTER};
  LeafPage *_prevPage{nullptr};
  LeafPage *_nextPage{nullptr};

  bool _bRangeEndPage{false};
  bool _bRangeBeginPage{false};

  friend class InsertAction;
};
} // namespace storage
