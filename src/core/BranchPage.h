#pragma once
#include "IndexPage.h"
#include "PageType.h"
#include "RawKey.h"

namespace storage {
class BranchRecord;

class BranchPage : public IndexPage {
public:
  static const uint16_t DATA_BEGIN_OFFSET;
  static const uint16_t MAX_DATA_LENGTH;

public:
  BranchPage(IndexTree *indexTree, uint32_t pageId, Byte pageLevel,
             uint32_t parentId)
      : IndexPage(indexTree, pageId, pageLevel, parentId,
                  PageType::BRANCH_PAGE) {}

  BranchPage(IndexTree *indexTree, uint32_t pageId)
      : IndexPage(indexTree, pageId, PageType::BRANCH_PAGE) {}
  ~BranchPage();

  bool SaveRecords() override;

  void CleanRecords();
  void LoadRecords();
  BranchRecord *DeleteRecord(uint16_t index);
  BranchRecord *DeleteRecord(const BranchRecord &record);

  void InsertRecord(BranchRecord *&record, int32_t pos = -1);
  bool AddRecord(BranchRecord *&record);
  bool RecordExist(const RawKey &key) const;

  int32_t SearchRecord(const BranchRecord &rr, bool &bFind) const;
  int32_t SearchKey(const RawKey &key, bool &bFind) const;
  BranchRecord *GetRecordByPos(int32_t pos, bool bAutoLast);

  bool IsPageFull() const { return _totalDataLength >= MAX_DATA_LENGTH; }
  uint16_t GetMaxDataLength() const override { return MAX_DATA_LENGTH; }
  void Init() override;

protected:
  inline BranchRecord *GetVctRecord(int pos) const {
    return (BranchRecord *)_vctRecord[pos];
  }
  int CompareTo(uint32_t recPos, const BranchRecord &rr) const;
  int CompareTo(uint32_t recPos, const RawKey &key) const;
};
} // namespace storage
