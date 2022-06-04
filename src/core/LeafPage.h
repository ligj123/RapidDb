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
  static uint16_t PREV_PAGE_POINTER_OFFSET;
  static uint16_t NEXT_PAGE_POINTER_OFFSET;
  static uint16_t DATA_BEGIN_OFFSET;
  static uint16_t MAX_DATA_LENGTH;

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
  inline bool IsPageFull() { return _totalDataLength >= MAX_DATA_LENGTH; }

  void LoadRecords();
  void CleanRecord();
  bool SaveRecords() override;

  /**Insert a leaf record to this page, if pos < 0, use SearchRecord to find the
  position, if pos>=0, insert the position or add to end; If passed to insert
  record to this page, return nullptr, else return error massage.*/
  ErrorMsg *InsertRecord(LeafRecord *lr, int32_t pos = -1);
  bool AddRecord(LeafRecord *record);
  void DeleteRecord(int32_t pos);
  void UpdateRecord(int32_t pos);
  void GetAllRecords(VectorLeafRecord &vct);
  /**
   * @brief Fetch the records from startKey to endKey
   * @return If include the last record, return true, or return false
   */
  bool FetchRecords(const RawKey *startKey, const RawKey *endKey,
                    const bool bIncLeft, const bool bIncRight,
                    VectorLeafRecord &vct);
  LeafRecord *GetLastRecord();
  LeafRecord *GetRecord(const RawKey &key);
  /**If include the last record, return true, or false*/
  bool GetRecords(const RawKey &key, VectorLeafRecord &vct,
                  bool bFromZero = false);
  int32_t SearchRecord(const LeafRecord &rr, bool &bFind, bool bInc = true,
                       int32_t start = 0, int32_t end = INT32_MAX);
  int32_t SearchKey(const RawKey &key, bool &bFind, bool bInc = true,
                    int32_t start = 0, int32_t end = INT32_MAX);
  int32_t SearchKey(const LeafRecord &rr, bool &bFind, bool bInc = true,
                    int32_t start = 0, int32_t end = INT32_MAX);
  uint16_t GetMaxDataLength() const override { return MAX_DATA_LENGTH; }

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
