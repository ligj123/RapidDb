#pragma once
#include "IndexPage.h"
#include "RawKey.h"

namespace storage {
class BranchRecord;

class BranchPage : public IndexPage {
public:
  static const uint16_t DATA_BEGIN_OFFSET;

public:
  // Create new brance page
  BranchPage(IndexTree *indexTree, uint32_t pageId, Byte pageLevel,
             uint32_t parentId)
      : IndexPage(indexTree, pageId, pageLevel, parentId,
                  PageType::BRANCH_PAGE) {}
  // Construct an existed branch page and send to read queue
  BranchPage(IndexTree *indexTree, uint32_t pageId)
      : IndexPage(indexTree, pageId, PageType::BRANCH_PAGE) {}
  ~BranchPage() {}

  void InitParameters() override;
  /**
   * @brief clear vector of records
   */
  void ClearRecords();
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords();
  /**
   * @brief Save records from vector and variable into buffer
   * @return If conditions is ok and saved successfully, return true, or false
   */
  bool SaveRecords() override;
  /**
   * @brief Delete the record at the index and return it.
   */
  BranchRecord *DeleteRecord(uint16_t index);
  /**
   * @brief Insert a record into the the pointed position
   * @param record The record to insert
   * @param pos The position to insert
   */
  void InsertRecord(BranchRecord *record, int32_t pos);
  /**
   * @brief Add the record into the last position of page
   * @param record The record to insert
   * @return True: success to insert into page; False: failed to insert into
   * page due to the page length exceeded the limit.
   */
  bool AddRecord(BranchRecord *record);
  /**
   * @brief To judge if a key exist
   */
  bool KeyExist(const RawKey &key) const;

  int32_t SearchRecord(const BranchRecord &rr, bool &bFind) const;
  int32_t SearchKey(const RawKey &key, bool &bFind) const;
  BranchRecord &GetRecord(int32_t pos, bool bAutoLast);

  void SetChild(int32_t pos, IndexPage *child);
  IndexPage *GetChild(int32_t pos);

  bool SplitPage(MHashSet<CachePage *> &pageSet,
                 Byte pageLevel = 0xFF) override;

  bool IsOverlength() override {
    return _committedDataLength >= MAX_DATA_LENGTH_BRANCH;
  }

protected:
  inline BranchRecord *GetVctRecord(int pos) const {
    return (BranchRecord *)_vctRecord[pos];
  }
  int CompareTo(uint32_t recPos, const BranchRecord &rr) const;
  int CompareTo(uint32_t recPos, const RawKey &key) const;

protected:
};
} // namespace storage
