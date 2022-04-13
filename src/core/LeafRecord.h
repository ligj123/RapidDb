#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/IDataValue.h"
#include "../transaction/TranStatus.h"
#include "../utils/BytesConvert.h"
#include "OverflowPage.h"
#include "RawKey.h"
#include "RawRecord.h"
#include <cstring>

using namespace std;
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
  // Constructor for primary index LeafRecord, only for insert
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp, ActionType type,
             Transaction *tran);
  // When insert or upsert, if there has old record in leaf page, replace old
  // record with this and call below method to set old record
  void SetUndoRecord(LeafRecord *undoRec);
  // When update this record, set new values into record and save old value into
  // _oldRec, only for primary index
  void UpdateRecord(const VectorDataValue &vctKey,
                    const VectorDataValue &vctVal, uint64_t recStamp,
                    ActionType type, Transaction *tran);
  // Call this function to delete a record, only for primary index
  void DeleteRecord(uint64_t recStamp, ActionType type, Transaction *tran);

  void GetListKey(VectorDataValue &vct) const;

  int GetListValue(VectorDataValue &vct, uint64_t verStamp = UINT64_MAX,
                   Transaction *tran = nullptr, bool bQuery = true) const;
  int GetListValue(const MVector<int> &vctPos, VectorDataValue &vct,
                   uint64_t verStamp = UINT64_MAX, Transaction *tran = nullptr,
                   bool bQuery = true) const;
  RawKey *GetKey() const;
  /**Only for secondary index, Get the value as primary key*/
  RawKey *GetPrimayKey() const;

  int CompareTo(const LeafRecord &lr) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const LeafRecord &lr) const;

  inline uint16_t GetValueLength() const override {
    return (*((uint16_t *)_bysVal) -
            *((uint16_t *)(_bysVal + sizeof(uint16_t))) - sizeof(uint16_t) * 2);
  }

  inline uint16_t SaveData(Byte *bysPage) {
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  bool IsSole() const override {
    return _undoRec == nullptr ? _bSole : _undoRec->IsSole();
  }
  bool IsTransaction() const { return _tran != nullptr; }

  inline void ReleaseRecord() {
    _refCount--;
    if (_refCount == 0)
      delete this;
  }
  inline LeafRecord *ReferenceRecord() {
    _refCount++;
    return this;
  }
  inline void LockForUpdate(Transaction *tran, ActionType type) {
    _tran = tran;
    _actionType = type;
  }

protected:
  // If update this record, save old version for transaction rollback. If it
  // has been updated more than one time. old rec can recursively contain an old
  // rec.
  LeafRecord *_undoRec = nullptr;
  // Write transaction, when insert, update or delete,
  Transaction *_tran = nullptr;
  // Vector to contain overflow page
  MVector<OverflowPage *>::Type _vctOvf;
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
