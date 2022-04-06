#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/IDataValue.h"
#include "../transaction/TranStatus.h"
#include "../utils/BytesConvert.h"
#include "RawKey.h"
#include "RawRecord.h"
#include <cstring>

namespace storage {
static const Byte LAST_OVERFLOW = 0x80;
static const Byte OTHER_OVERFLOW = 0x40;
static const Byte VERSION_NUM = 0x0f;

class LeafPage;
class Transaction;
class LeafRecord : public RawRecord {
protected:
  ~LeafRecord();

public:
  // Load LeafRecord from LeafPage
  LeafRecord(LeafPage *indexPage, Byte *bys);
  // No use now, only for test
  LeafRecord(IndexTree *indexTree, Byte *bys);
  LeafRecord(const LeafRecord &src) = delete;
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType type, Transaction *tran);
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
  inline void AddTrasaction(Transaction *tran, bool bWrite) { _tran = tran; }

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
  struct LeafTran {
    ~LeafTran() {}
    // If update this record, save old version for transaction rollback. If it
    // has been updated more than one time. old rec can contain more old rec.
    LeafRecord *_oldRec = nullptr;
    // Write transaction
    Transaction *_tranWrite = nullptr;
    //
    MHashMap<uint64_t, Transaction *>::Type _vctTranRead;
  }

  LeafTran *_leafTran = nullptr;
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
