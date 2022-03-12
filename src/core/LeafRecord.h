﻿#pragma once
#include "../dataType/IDataValue.h"
#include "../transaction/TranStatus.h"
#include "../utils/BytesConvert.h"
#include "RawKey.h"
#include "RawRecord.h"
#include <cstring>

namespace storage {
static const Byte LAST_RECORD_OVERFLOW = 0x80;
static const Byte RECORD_OVERFLOW = 0x40;
static const Byte RECORD_DELETED = 0x20;

struct PriValStruct {
public:
  PriValStruct() { std::memset(this, 0, sizeof(PriValStruct)); }

  PriValStruct(Byte *bys) {
    std::memset(this, 0, sizeof(PriValStruct));
    uint16_t pos = sizeof(uint16_t) * 2 + *(uint16_t *)&bys[sizeof(uint16_t)];
    bLastOvf = bys[pos] & LAST_RECORD_OVERFLOW;
    bOvf = bys[pos] & RECORD_OVERFLOW;
    bDelete = bys[pos] & RECORD_DELETED;
    verCount = bys[pos] & 0xf;
    pos++;
    arrStamp = (uint64_t *)&bys[pos];
    pos += sizeof(uint64_t) * verCount;

    if (bOvf) {
      ovfOffset = *(uint64_t *)&bys[pos];
      pos += sizeof(uint64_t);
      ovfRange = *(uint32_t *)&bys[pos];
      pos += sizeof(uint32_t);
      indexOvfStart = *(uint16_t *)&bys[pos];
      pos += sizeof(uint16_t);
      lenInPage = *(uint16_t *)&bys[pos];
      pos += sizeof(uint16_t);
      arrOvfLen = (uint32_t *)&bys[pos];
      pageValOffset = pos + sizeof(uint32_t) * verCount;
    } else {
      arrPageLen = (uint16_t *)&bys[pos];
      pageValOffset = pos + sizeof(uint16_t) * verCount;
    }
  }

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  bool bLastOvf;
  bool bOvf;
  bool bDelete; // If the last snapshot has been deleted
  Byte verCount;
  uint16_t pageValOffset;
  uint64_t *arrStamp;
  union {
    uint16_t *arrPageLen;
    struct {
      uint64_t ovfOffset;
      uint32_t ovfRange;
      uint16_t indexOvfStart;
      uint16_t lenInPage;
      uint32_t *arrOvfLen;
    };
  };
};

class LeafPage;
class Transaction;
class LeafRecord : public RawRecord {
protected:
  ~LeafRecord();

public:
  // Load LeafRecord from LeafPage
  LeafRecord(LeafPage *indexPage, Byte *bys);
  // No used now, only for test
  LeafRecord(IndexTree *indexTree, Byte *bys);
  LeafRecord(const LeafRecord &src) = delete;
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType type = ActionType::UNKNOWN,
             Transaction *tran = nullptr);
  // Constructor for primary index LeafRecord
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp,
             ActionType type = ActionType::UNKNOWN, Transaction *tran = nullptr,
             LeafRecord *old = nullptr);
  /**To calc the length of snapshot for multi versions' record. To add last
  version record or not,
  decide by its record stamp < last version stamp in head page or not*/
  uint32_t GetSnapshotLength() const;

  inline void ReleaseRecord() {
    _refCount--;
    if (_refCount == 0)
      delete this;
  }
  inline LeafRecord *ReferenceRecord() {
    _refCount++;
    return this;
  }
  inline ActionType GetActionType() { return _actionType; }
  inline Transaction *GetTrasaction() { return _tran; }
  inline void SetActionType(ActionType type) { _actionType = type; }
  inline void SetTrasaction(Transaction *tran) { _tran = tran; }

  void GetListKey(VectorDataValue &vct) const;
  /**
   * @brief Read the value to data value list
   * @param vct The vector to save the data values.
   * @param verStamp The version stamp for primary index.If not primary, not to
   * use.
   * @param tran To judge if it is the same transaction
   * @param bQuery If it is only query. If not only query, it will return record
   * with same transaction or return fail. If only query, it will return old
   * record with different transaction and new record with same transaction.
   * @return -1: Failed to read values due to no right version stamp for
   * primary or locked; 0: Passed to read values with all fields. 1: Passed to
   read part
   * of values due to same of fields saved in overflow file.
   */
  int GetListValue(VectorDataValue &vct, uint64_t verStamp = UINT64_MAX,
                   Transaction *tran = nullptr, bool bQuery = true) const;
  void GetListOverflow(VectorDataValue &vctVal) const;

  int CompareTo(const LeafRecord &lr) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const LeafRecord &lr) const;
  RawKey *GetKey() const;
  /**Only for secondary index, Get the value as primary key*/
  RawKey *GetPrimayKey() const;

  inline uint16_t GetValueLength() const override {
    return (*((uint16_t *)_bysVal) -
            *((uint16_t *)(_bysVal + sizeof(uint16_t))) - sizeof(uint16_t) * 2);
  }

  inline uint16_t GetIndexOvfStart() const {
    return (GetPriValStruct()->bLastOvf) ? GetPriValStruct()->indexOvfStart
                                         : UINT16_MAX;
  }

  inline uint16_t SaveData(Byte *bysPage) {
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  inline PriValStruct *GetPriValStruct() const {
    if (_priStru == nullptr) {
      _priStru = new PriValStruct(_bysVal);
    }

    return _priStru;
  }
  bool IsSole() const override {
    return _undoRecords == nullptr ? _bSole : _undoRecords->IsSole();
  }
  bool IsTransaction() const override { return _tran != nullptr; }

protected:
  /**Save old records for recover*/
  LeafRecord *_undoRecords = nullptr;
  /**The transaction own the write lock for this record, or nullptr without
   * write lock*/
  Transaction *_tran = nullptr;
  mutable PriValStruct *_priStru = nullptr;

  friend std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
  friend bool operator<(const LeafRecord &llr, const LeafRecord &rlr);
};

inline bool operator<(const LeafRecord &llr, const LeafRecord &rlr) {
  return llr < rlr;
}

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
class VectorLeafRecord : public MVector<LeafRecord *>::Type {
public:
  using vector::vector;
  ~VectorLeafRecord() {
    for (auto iter = begin(); iter != end(); iter++) {
      (*iter)->ReleaseRecord();
    }
  }

  void RemoveAll() {
    for (auto iter = begin(); iter != end(); iter++) {
      (*iter)->ReleaseRecord();
    }

    clear();
  }
};
} // namespace storage
