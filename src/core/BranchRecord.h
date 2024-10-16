﻿#pragma once
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
  /**Page Id length*/
  static const uint32_t PAGE_ID_LEN;

public:
  BranchRecord(IndexType type, Byte *bys) : RawRecord(bys, false, type) {}

  BranchRecord(IndexType type, RawRecord *rec, uint32_t childPageId);
  BranchRecord(BranchRecord &&src)
      : RawRecord(std::move(src)), _childPage(src._childPage) {
    src._childPage = nullptr;
  }
  BranchRecord(const BranchRecord &src) = delete;
  BranchRecord() : RawRecord() {}

  ~BranchRecord() {}

  BranchRecord &operator=(BranchRecord &&src) {
    _bysVal = src._bysVal;
    src._bysVal = nullptr;
    _bSole = src._bSole;
    _indexType = src._indexType;
    _childPage = src._childPage;
    src._childPage = nullptr;
    return *this;
  }
  BranchRecord &operator=(const BranchRecord &src) = delete;

  int CompareTo(const RawRecord &other, IndexType type) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const RawRecord &other) const;
  bool EqualPageId(const BranchRecord &br) const;

  uint16_t GetTotalLength() const override { return *((uint16_t *)_bysVal); }
  uint16_t GetValueLength() const override {
    return (uint16_t)(*((uint16_t *)_bysVal) - UI16_2_LEN - PAGE_ID_LEN -
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
  IndexPage *GetChildPage() { return _childPage; }

protected:
  IndexPage *_childPage;
  friend std::ostream &operator<<(std::ostream &os, const BranchRecord &br);
};

std::ostream &operator<<(std::ostream &os, const BranchRecord &br);
} // namespace storage
