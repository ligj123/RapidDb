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

// Only for primary index
struct RecStruct {
  RecStruct(Byte *bys, uint16_t varKeyOff, uint16_t keyLen, Byte verNum,
            OverflowPage *overPage = nullptr);
  RecStruct(Byte *bys, uint16_t varKeyOff, OverflowPage *overPage = nullptr);

  // To save total length for record, not include values in overflow page
  // length, only to calc the occupied bytes in LeafPage
  uint16_t *_totalLen;
  // To save key length, include the bytes to save variable fileds length and
  // key content.
  uint16_t *_keyLen;
  // The start position to save variable fileds length in key, this is an array
  // with 0~N elements.
  uint16_t *_varKeyLen;
  // To save key content after serialize.
  Byte *_bysKey;
  // The byte to save the number of versions(low 4bits) and if overflow or not
  // (the highest bit)
  Byte *_byVerNum;
  // The array to save version stamps for every version, 1~N elements.
  uint64_t *_arrStamp;
  // The array to save length for every version content.
  uint32_t *_arrValLen;
  // The array to save the crc32 if overflow
  uint32_t *_arrCrc32;
  // The start page id
  PageID *_pidStart;
  // The page number
  uint16_t *_pageNum;
  // The start pointer to save values
  Byte *_bysValStart;
};

struct ValueStruct {
  // (n+7)/8 bytes to save if the related fields are null or not, every field
  // occupys a bit.
  Byte *bysNull;
  // The variable fields' lengths after serialize
  uint32_t *varFiledsLen;
  // Start position to save value content
  Byte *bysValue;
};

void SetValueStruct(RecStruct &recStru, ValueStruct *arvalStr,
                    uint32_t fieldNum, uint32_t valVarLen) {
  Byte verNum = (*recStru._byVerNum) & 0x0f;
  uint32_t byNum = (fieldNum + 7) << 3;
  Byte *bys = recStru._bysValStart;
  uint32_t offset = 0;
  for (Byte i = 0; i < verNum; i++) {
    arvalStr[i].bysNull = bys + offset;
    arvalStr[i].varFiledsLen = (uint32_t *)(bys + offset + byNum);
    arvalStr[i].bysValue = bys + offset + byNum + valVarLen;
    offset += recStru._arrValLen[i];
  }
}

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
  LeafRecord(LeafRecord &src);
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType type, Transaction *tran);
  // Constructor for primary index LeafRecord, only for insert
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp,
             Transaction *tran);

  // When update or delete this record, set new values into record and save old
  // value into _undoRec, only for primary index
  void UpdateRecord(const VectorDataValue &newVal, uint64_t recStamp,
                    Transaction *tran, ActionType type);

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

  inline void ReleaseRecord(bool bUndo = false) {
    _refCount--;
    if (_refCount == 0) {
      if (_overflowPage != nullptr) {
        if (_bUnRec) {
          _indexTree->ReleasePageId(_overflowPage->GetPageId(),
                                    _overflowPage->GetPageNum());
        }
        _overflowPage->DecRefCount();
      }
      delete this;
    }
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
    for (size_t i = 0; i < vctVal.size(); i++) {
      lenVal += vctVal[i]->GetPersistenceLength(false);
    }
    return lenVal;
  }

  // Save key and infor into buffer
  inline void FillHeaderBuff(RecStruct &recStru, uint32_t totalLen,
                             uint32_t keyLen, Byte verNum, uint64_t stamp,
                             uint32_t valLen) {
    *recStru._totalLen = totalLen;
    *recStru._keyLen = keyLen;
    *recStru._byVerNum = (recStru._pidStart == nullptr ? 0x80 : 0) + verNum;
    recStru._arrStamp[0] = stamp;
    recStru._arrValLen[0] = valLen;
  }

  inline void FillKeyBuff(RecStruct &recStru, const VectorDataValue &vctKey) {
    Byte *bys = recStru._bysKey;
    uint16_t *varLen = recStru._varKeyLen;

    for (size_t i = 0; i < vctKey.size(); i++) {
      uint16_t vl = vctKey[i]->WriteData(bys, true);
      bys += vl;
      if (!vctKey[i]->IsFixLength()) {
        *varLen = vl;
        varLen++;
      }
    }
  }

  // Save a version's value into buffer
  inline void FillValueBuff(ValueStruct &valStru,
                            const VectorDataValue &vctVal) {
    memset(valStru.bysNull, 0, (vctVal.size() + 7) >> 3);
    uint32_t *vlen = valStru.varFiledsLen;
    Byte *bys = valStru.bysValue;
    for (size_t i = 0; i < vctVal.size(); i++) {
      if (vctVal[i]->IsNull()) {
        valStru.bysNull[i / 8] |= 1 << (i % 8);
      }
      uint32_t vl = vctVal[i]->WriteData(bys, true);
      bys += vl;
      if (!vctVal[i]->IsFixLength()) {
        *vlen = vl;
        vlen++;
      }
    }
  }

  /**
   * @brief To calc the value length for move to new record. Here will remove
   * the repeated version
   * @param bUpdate True: update this record and will judge the last version if
   * repeate with new version. False: only remove repeated version
   * @param vctSN To save the serial number that will moved to new record
   * @return The total length of versions will been moved new record
   */
  inline uint32_t CalcValidValueLength(RecStruct &recStru, bool bUpdate,
                                       MVector<Byte>::Type &vctSN);

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
