#pragma once
#include "../dataType/IDataValue.h"
#include "../utils/BytesFuncs.h"
#include "RawKey.h"
#include "RawRecord.h"

#include <cstring>
#include <utility>

namespace storage {
class IndexPage;
class BranchRecord : public RawRecord {
public:
  /** Construct an exist record from page buffer */
  BranchRecord(IndexType type, Byte *bys, IndexPage *childPage = nullptr)
      : RawRecord(bys, false, type), _childPage(childPage) {}
  /** Create a new record */
  BranchRecord(IndexType type, RawRecord *rec, uint32_t childPageId,
               IndexPage *childPage = nullptr);
  BranchRecord(BranchRecord &&src) = delete;
  BranchRecord(const BranchRecord &src) = delete;
  BranchRecord() : RawRecord() {}

  ~BranchRecord();

  BranchRecord &operator=(BranchRecord &&src) = delete;
  BranchRecord &operator=(const BranchRecord &src) = delete;

  void Copy(const BranchRecord &src);
  int CompareTo(const RawRecord &other) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const RawRecord &other) const;
  bool EqualPageId(const BranchRecord &br) const;

  uint16_t GetTotalLength() const override { return *((uint16_t *)_bysVal); }
  uint16_t GetValueLength() const override {
    if (_indexType == IndexType::NON_UNIQUE) {
      return (uint16_t)(*((uint16_t *)_bysVal) - UI16_2_LEN - PAGE_ID_LEN -
                        *((uint16_t *)(_bysVal + sizeof(uint16_t))));
    } else {
      return 0;
    }
  }
  uint16_t GetDataLength() const override {
    assert(_indexType == IndexType::NON_UNIQUE);
    return (uint16_t)(*((uint16_t *)_bysVal) - UI16_2_LEN - PAGE_ID_LEN);
  }
  PageID GetChildPageId() const {
    return *((PageID *)(_bysVal + GetTotalLength() - PAGE_ID_LEN));
  }
  uint16_t SaveData(Byte *bysPage) {
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }
  IndexPage *GetChildPage() { return _childPage; }
  void SetChildPage(IndexPage *child) { _childPage = child; }

protected:
  IndexPage *_childPage;
  friend std::ostream &operator<<(std::ostream &os, const BranchRecord &br);
};

std::ostream &operator<<(std::ostream &os, const BranchRecord &br);
} // namespace storage
