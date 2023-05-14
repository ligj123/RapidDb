#pragma once
#include "../utils/ErrorMsg.h"
#include "HeadPage.h"
#include "IndexPage.h"
#include "RawKey.h"

namespace storage {
class LeafRecord;
class VectorLeafRecord;

class LeafPage : public IndexPage {
public:
  static const uint16_t PREV_PAGE_POINTER_OFFSET;
  static const uint16_t NEXT_PAGE_POINTER_OFFSET;
  static const uint16_t DATA_BEGIN_OFFSET;
  static void RollbackLeafRecords(const MVector<LeafRecord *> &vctRec,
                                  int64_t endPos);

public:
  void clear() {
    CleanRecord();
    _recordNum = 0;
    _totalDataLength = 0;
  }
  LeafPage(IndexTree *indexTree, PageID pageId, PageID parentPageId);
  LeafPage(IndexTree *indexTree, PageID pageId);
  ~LeafPage();
  void Init() override;

  inline void SetPrevPageId(PageID id) { _prevPageId = id; }
  inline PageID GetPrevPageId() { return _prevPageId; }
  inline void SetNextPageId(PageID id) { _nextPageId = id; }
  inline PageID GetNextPageId() { return _nextPageId; }
  inline bool IsPageFull() { return _totalDataLength >= MAX_DATA_LENGTH_LEAF; }

  void LoadRecords();
  void CleanRecord();
  bool SaveRecords() override;
  /**
   * @brief Insert a leaf record into position pos in this page
   *
   * @param lr The leaf record will be inserted
   * @param pos The position for insert.
   * @param incRef Increase lr reference times or not
   */
  void InsertRecord(LeafRecord *lr, int32_t pos, bool incRef = false);
  /**
   * @brief For test aim, if insert fail will put the error message into
   * _threadErrorMsg
   * @param lr The leaf record will be inserted
   * @param incRef Increase lr reference times or not
   * @return True: succeed to insert the record; False: failed to insert and set
   * the failed reason into ErrorMsg::_threadErrorMsg
   */
  bool InsertRecord(LeafRecord *lr, bool incRef = false) {
    bool bFind;
    int32_t pos = SearchRecord(*lr, bFind);
    if (bFind) {
      _threadErrorMsg.reset(new ErrorMsg(CORE_REPEATED_RECORD, {}));
      return false;
    }

    InsertRecord(lr, pos, incRef);
    return true;
  }
  /** @brief Add a new record to the last position of this page. Only used wehn
   * batch add for ordered records.
   * @param record The new record
   * @return True: passed to add the record; False: failed to add the record due
   * to reach length limit.
   */
  bool AddRecord(LeafRecord *record);
  /**
   * @brief Get the Record in this LeafPage with position=pos   *
   * @param pos The position of records in this page
   * @return LeafRecord* The leaf record to get
   */
  LeafRecord *GetRecord(int32_t pos);
  int32_t SearchRecord(const LeafRecord &rr, bool &bFind, int32_t start = 0,
                       int32_t end = INT32_MAX);
  int32_t SearchKey(const RawKey &key, bool &bFind, int32_t start = 0,
                    int32_t end = INT32_MAX);
  int32_t SearchKey(const LeafRecord &rr, bool &bFind, int32_t start = 0,
                    int32_t end = INT32_MAX);
  void UpdateTotalLength(int32_t len) { _totalDataLength += len; }

protected:
  inline LeafRecord *GetVctRecord(int pos) const {
    return (LeafRecord *)_vctRecord[pos];
  }
  int CompareTo(uint32_t recPos, const RawKey &key);
  int CompareTo(uint32_t recPos, const LeafRecord &rr, bool key);

protected:
  uint32_t _prevPageId;
  uint32_t _nextPageId;
};
} // namespace storage
