#pragma once
#include "IndexPage.h"
#include "RawKey.h"

namespace storage {
class BranchRecord;
class LeafPage;

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
  ~BranchPage() { ClearRecords(); }

  void InitParameters() override;
  /**
   * @brief clear vector of records
   */
  void ClearRecords() override;
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords() override;
  /**
   * @brief Save records from vector and variable into buffer
   * @return If conditions is ok and saved successfully, return true, or false
   */
  bool SaveRecords();
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
  bool AppendRecord(BranchRecord *record);

  int32_t SearchRecord(const RawRecord &rr) const;
  int32_t SearchKey(const RawKey &key) const;
  BranchRecord &GetRecord(int32_t pos, bool bAutoLast);

  void SetChild(int32_t pos, IndexPage *child);
  IndexPage *GetChild(int32_t pos);
  /**
   * @brief Find the BranchRecord included this child and clear child page in
   * the BranchRecord
   * @param child The IndexPage need to clear
   */
  void ClearChild(IndexPage *child);

  bool SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap) override;

  bool IsOverlength() override {
    return _committedDataLength >= MAX_DATA_LENGTH_BRANCH;
  }
  inline BranchRecord *GetVctRecord(int pos) const {
    return (BranchRecord *)_vctRecord[pos];
  }

  LeafPage *GetLeftLeafChild();
  LeafPage *GetRightLeafChild();

  IndexPage *GetNextPage(IndexPage *currPage);
  /**
   * @brief Fill _nextPage for following brother pages of currPage. If this
   * page is the last page in this parent page, it will fill the children pages
   * in the next page of this page.
   * @param currPage The LeafPage that need to fill next page
   * @param bAll True: Fill all next page with same parent page
   *             False: Only fill the pages follow current pages in same parent
   *                    page.
   */
  void FillNextPage(IndexPage *currPage, bool bAll = false);

protected:
};
} // namespace storage
