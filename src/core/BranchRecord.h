#pragma once
#include "../dataType/IDataValue.h"
#include "../utils/BytesConvert.h"
#include "RawKey.h"
#include "RawRecord.h"
#include <cstring>

namespace storage {
class BranchPage;
class BranchRecord : public RawRecord {
public:
  /**Page Id length*/
  static const uint32_t PAGE_ID_LEN;

public:
  BranchRecord(BranchPage *parentPage, Byte *bys);
  BranchRecord(IndexTree *indexTree, RawRecord *rec, uint32_t childPageId);
  BranchRecord(const BranchRecord &src) = delete;
  ~BranchRecord() {}

  RawKey *GetKey() const;
  void GetListKey(VectorDataValue &vct) const;
  void GetListValue(VectorDataValue &vct) const;
  int CompareTo(const RawRecord &other) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const RawRecord &other) const;
  bool EqualPageId(const BranchRecord &br) const;

  inline void ReleaseRecord() {
    _refCount--;
    if (_refCount == 0)
      delete this;
  }
  inline BranchRecord *ReferenceRecord() {
    _refCount++;
    return this;
  }
  uint16_t GetValueLength() const override {
    if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE)
      return 0;

    return (uint16_t)(*((uint16_t *)_bysVal) - TWO_SHORT_LEN - PAGE_ID_LEN -
                      *((uint16_t *)(_bysVal + sizeof(uint16_t))));
  }

  PageID GetChildPageId() const {
    return *((PageID *)(_bysVal + GetTotalLength() - PAGE_ID_LEN));
  }
  uint16_t SaveData(Byte *bysPage) {
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  friend std::ostream &operator<<(std::ostream &os, const BranchRecord &br);
};

std::ostream &operator<<(std::ostream &os, const BranchRecord &br);
} // namespace storage
