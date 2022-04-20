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
             const VectorDataValue &vctVal, uint64_t recStamp,
             Transaction *tran);

  // When update this record, set new values into record and save old value into
  // _undoRec, only for primary index
  void UpdateRecord(const VectorDataValue &newVal, uint64_t recStamp,
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

  // When insert or upsert, if there has old record in leaf page, replace old
  // record with this and call below method to set old record
  void SaveUndoRecord(LeafRecord *undoRec) { _undoRec = undoRec; }
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
  inline bool Releaseable() {
    return !(_refCount > 1 ||
             (_overflowPage != nullptr && _overflowPage->IsDirty()));
  }

protected:
  // To calc key length
  inline uint32_t CalcKeyLength(const VectorDataValue &vctKey) {
    uint32_t lenKey = _indexTree->GetKeyVarLen();
    for (int i = 0; i < vctKey.size(); i++) {
      lenKey += vctKey[i]->GetPersistenceLength(true);
    }

    if (lenKey > Configure::GetMaxKeyLength()) {
      throw ErrorMsg(CORE_EXCEED_KEY_LENGTH, {std::to_string(lenKey)});
    }
    return lenKey;
  }
  // To calc a version's value length
  inline uint32_t CalcValueLength(const VectorDataValue &vctVal,
                                  ActionType type) {
    if (type == ActionType::DELETE)
      return 0;

    uint32_t lenVal = (vctVal.size() + 7) / 8 + _indexTree->GetValVarLen();
    for (i = 0; i < vctVal.size(); i++) {
      lenVal += vctVal[i]->GetPersistenceLength(false);
    }
    return lenVal;
  }

  // Save key into buffer and return current buffer pointer
  inline Byte *FillKeyBuff(Byte *bys, uint32_t totalLen, uint32_t keyLen,
                           const VectorDataValue &vctKey) {
    *((uint16_t *)bys) = totalLen;
    bys += UI16_LEN;
    *((uint16_t *)bys) = lenKey;
    bys += UI16_LEN;
    Byte *bys2 = bys + _indexTree->GetKeyVarLen();
    for (i = 0; i < vctKey.size(); i++) {
      uint16_t vl = vctKey[i]->WriteData(bys2, true);
      bys2 += vl;
      if (!vctKey[i]->IsFixLength()) {
        *((uint16_t *)bys) = vl;
        bys += UI16_LEN;
      }
    }

    return bys2;
  }

  // Save a version's value into buffer
  inline Byte *FillValueBuff(Byte *buf, const VectorDataValue &vctVal) {
    Byte *fBit = buf;
    Byte *bys = fBit + (vctVal.size() + 7) / 8;
    Byte *bys2 = bys + UI16_3_LEN + _indexTree->GetValVarLen();
    memset(fBit, 0, (vctVal.size() + 7) / 8);
    for (i = 0; i < vctVal.size(); i++) {
      fBit[i / 8] |= vctVal[i]->IsNull() ? (1 << (i % 8)) : 0;
      uint32_t vl = vctKey[i]->WriteData(bys2, true);
      bys2 += vl;
      if (!vctKey[i]->IsFixLength()) {
        *((uint16_t *)bys) = vl;
        bys += UI16_LEN;
      }
    }
  }

protected:
  // If update this record, save old version for transaction rollback. If it
  // has been updated more than one time. old rec can recursively contain an
  // old rec.
  LeafRecord *_undoRec = nullptr;
  // Write transaction, when insert, update or delete,
  Transaction *_tran = nullptr;
  // Vector to contain overflow page
  OverflowPage *_overflowPage = nullptr;
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
