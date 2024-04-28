#pragma once
#include "IndexPage.h"
#include "PageType.h"
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
  // Create for existed branch page and send to read queue
  BranchPage(IndexTree *indexTree, uint32_t pageId)
      : IndexPage(indexTree, pageId, PageType::BRANCH_PAGE) {}
  ~BranchPage() {}
  /**
   * @brief Save records from vector and variable into buffer
   * @return If conditions is ok and saved successfully, return true, or false
   */
  bool SaveRecords() override;
  /**
   * @brief clear vector of records and children
   */
  void CleanRecords();
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords();
  void DeleteRecord(uint16_t index, BranchRecord &brDel);

  void InsertRecord(BranchRecord *&record, int32_t pos);
  bool AddRecord(BranchRecord *&record);
  bool RecordExist(const RawKey &key) const;

  int32_t SearchRecord(const BranchRecord &rr, bool &bFind) const;
  int32_t SearchKey(const RawKey &key, bool &bFind) const;
  BranchRecord *GetRecordByPos(int32_t pos, bool bAutoLast);
  bool SplitPage() override;

protected:
  inline BranchRecord *GetVctRecord(int pos) const {
    return (BranchRecord *)_vctRecord[pos];
  }
  int CompareTo(uint32_t recPos, const BranchRecord &rr) const;
  int CompareTo(uint32_t recPos, const RawKey &key) const;
};
} // namespace storage
