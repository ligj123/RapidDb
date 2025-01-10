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

  LeafPage *GetPrevPage();
  LeafPage *GetNextPage();

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
  // bool InsertRecord(LeafRecord *lr) {
  //   bool bFind;
  //   int32_t pos = SearchRecord(*lr, bFind);
  //   if (bFind) {
  //     _threadErrorMsg.reset(new ErrorMsg(CORE_REPEATED_RECORD, {}));
  //     return false;
  //   }

  //   InsertRecord(lr, pos);
  //   return true;
  // }

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
   * @return True: passed to add the record; False: failed to add the record due
   * to reach length limit.
   */
  bool AddRecord(LeafRecord *record);
  /**
   * @brief Save the records content into byte array
   * @param pageSet If there has OverflowPages that need to write disk, add into
   * this set
   * @param block If there have multi thread tasks for this index set it to true
   * @return True: The page is clean and all data has been saved into buffer;
   * False: The page has dirty data and can not save all of data into buffer.
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

  void ClearRecords() override;
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords() override;
  bool SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap,
                 Byte lockPageLevel = UINT8_MAX) override;

protected:
  uint32_t _prevPageId{PAGE_NULL_POINTER};
  uint32_t _nextPageId{PAGE_NULL_POINTER};
  LeafPage *_prevPage{nullptr};
  LeafPage *_nextPage{nullptr};

  friend class InsertAction;
};
} // namespace storage
