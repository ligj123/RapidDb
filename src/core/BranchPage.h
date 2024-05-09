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

  void InitParameters() override;
  /**
   * @brief clear vector of records
   */
  void ClearRecords() override;
  /**
   * @brief Load records from buffer into vector and reset children
   */
  void LoadRecords();
  /**
   * @brief Save records from vector and variable into buffer
   * @return If conditions is ok and saved successfully, return true, or false
   */
  bool SaveToBuffer() override;
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
  bool RecordExist(const RawKey &key) const;

  int32_t SearchRecord(const BranchRecord &rr, bool &bFind) const;
  int32_t SearchKey(const RawKey &key, bool &bFind) const;
  const BranchRecord &GetRecord(int32_t pos, bool bAutoLast);

  void SetChild(int32_t pos, IndexPage *child) {
    assert(pos > 0 && pos < _recordNum);
    assert(_children.size() == 0 || _children.size() == _recordNum);
    if (_children.size() == 0) {
      _children.resize(_recordNum, nullptr);
    }
    _children[pos] = child;
  }
  void InsertChild(int32_t pos, IndexPage *child) {
    assert(pos > 0 && pos < _recordNum);
    _children[pos] = child;
    _children.insert(child, _children.begin() + pos);
  }
  IndexPage *GetChild(int32_t pos) {
    assert(pos > 0 && pos < _recordNum);
    assert(_children.size() == 0 || _children.size() == _recordNum);
    if (_children.size() == 0) {
      return nullptr;
    }
    return _children[pos];
  }
  void ClearChildren() {
    for (auto page : _children) {
      page->SetReferred(false); //?
    }
    _children.clear();
  }
  bool IsOverlength() override {
    return _totalDataLength >= MAX_DATA_LENGTH_BRANCH;
  }

protected:
  inline BranchRecord *GetVctRecord(int pos) const {
    return (BranchRecord *)_vctRecord[pos];
  }
  int CompareTo(uint32_t recPos, const BranchRecord &rr) const;
  int CompareTo(uint32_t recPos, const RawKey &key) const;

protected:
  MVector<IndexPage *> _children;
};
} // namespace storage
