#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/IDataValue.h"
#include "../transaction/TranStatus.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorMsg.h"
#include "OverflowPage.h"
#include "RawKey.h"
#include "RawRecord.h"
#include <cstring>

using namespace std;
namespace storage {
static const Byte REC_OVERFLOW = 0x80;
static const Byte VERSION_NUM = 0x0f;

// Only for primary index
struct RecStruct {
  RecStruct(Byte *bys, uint16_t keyLen, Byte verNum,
            OverflowPage *overPage = nullptr);

  // Load record struct from byte array,
  RecStruct(Byte *bys, OverflowPage *overPage);

  // To save total length for record, not include values in overflow page
  // length, only to calc the occupied bytes in LeafPage
  uint16_t *_totalLen;
  // To save key length, include the bytes to save variable fileds length and
  // key content.
  uint16_t *_keyLen;
  // The byte to save the number of versions(low 4bits) and if overflow or not
  // (the highest bit)
  Byte *_byVerNum;
  // The start position to save variable fileds length in key, this is an array
  // with 0~N elements.
  // uint16_t *_varKeyLen;
  // To save key content after serialize.
  Byte *_bysKey;
  // The array to save version stamps for every version, 1~N elements.
  uint64_t *_arrStamp;
  // The array to save length for every version content.
  uint32_t *_arrValLen;
  // The address to save the crc32 if it has overflow page
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
                    uint32_t fieldNum, uint32_t valVarLen, Byte ver = 0xff);

class LeafPage;
class Statement;
class LeafRecord : public RawRecord {
protected:
  ~LeafRecord() {}

public:
  // Load LeafRecord from LeafPage
  LeafRecord(LeafPage *indexPage, Byte *bys);
  // No use now, only for test
  LeafRecord(IndexTree *indexTree, Byte *bys);
  LeafRecord(LeafRecord &src);
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType type, Statement *stmt);
  // Constructor for primary index LeafRecord, only for insert
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp, Statement *stmt);

  int32_t UpdateRecord(const VectorDataValue &newVal, uint64_t recStamp,
                       Statement *stmt, ActionType type, bool gapLock);

  // void GetListKey(VectorDataValue &vct) const;

  inline int GetListValue(VectorDataValue &vct, uint64_t verStamp = UINT64_MAX,
                          Statement *stmt = nullptr, bool bQuery = true) const {
    return GetListValue({}, vct, verStamp, stmt, bQuery);
  }
  int GetListValue(const MVector<int> &vctPos, VectorDataValue &vct,
                   uint64_t verStamp = UINT64_MAX, Statement *stmt = nullptr,
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
  LeafRecord *GetUndoRecord() { return _undoRec; }

  // Get the value's length. If there have more than one version, return the
  // first version's length.
  uint16_t GetValueLength() const override;

  inline uint16_t SaveData(Byte *bysPage) {
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  bool IsSole() const override {
    return _undoRec == nullptr ? _bSole : _undoRec->IsSole();
  }
  bool IsTransaction() const override { return _statement != nullptr; }

  inline LeafRecord *AddRef() {
    _refCount++;
    assert(_refCount < UINT16_MAX - 1);
    return this;
  }
  void DecRef(bool bUndo = false);

  inline void LockForUpdate(Statement *stam, bool gapLock) {
    _statement = stam;
    _actionType = ActionType::QUERY_UPDATE;
    _gapLock = gapLock;
  }
  inline void ClearLock() {
    _statement = nullptr;
    _actionType = ActionType::UNKNOWN;
    _gapLock = false;
  }
  bool IsTransaction() {
    return _statement != nullptr && _actionType != ActionType::UNKNOWN;
  }
  bool IsGapLock() { return _statement != nullptr && _gapLock; }
  bool FillOverPage() {
    uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);
    Byte ver = *(_bysVal + UI16_2_LEN + keyLen);
    if ((ver & REC_OVERFLOW) == 0 || _overflowPage != nullptr)
      return true;

    ver = ver & VERSION_NUM;
    Byte *bys =
        _bysVal + UI16_2_LEN + keyLen + 1 + UI64_LEN * ver + UI32_LEN * ver * 2;
    PageID pid = *(PageID *)(bys);
    uint16_t pnum = *(uint16_t *)(bys + UI32_LEN);

    _overflowPage = OverflowPage::GetPage(_indexTree, pid, pnum, false);
    return (_overflowPage->GetPageStatus() == PageStatus::VALID);
  }

  Byte GetVersionNumber() const {
    uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);
    return *(_bysVal + UI16_2_LEN + keyLen) & VERSION_NUM;
  }

  void GetVerStamps(MVector<uint64_t> &vctStamp) {
    uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);
    Byte ver = GetVersionNumber();
    uint64_t *arrStamp = (uint64_t *)(_bysVal + UI16_2_LEN + keyLen + 1);
    vctStamp.clear();

    for (Byte ii = 0; ii < ver; ii++) {
      vctStamp.push_back(arrStamp[ii]);
    }
  }
  ActionType GetAction() { return _actionType; }

protected:
  // To calc key length
  inline uint32_t CalcKeyLength(const VectorDataValue &vctKey) {
    uint32_t lenKey = 0;
    for (int i = 0; i < vctKey.size(); i++) {
      lenKey += vctKey[i]->GetPersistenceLength(SavePosition::KEY);
    }

    if (lenKey > Configure::GetMaxKeyLength()) {
      throw ErrorMsg(CORE_EXCEED_KEY_LENGTH, {to_string(lenKey)});
    }
    return lenKey;
  }
  // To calc a version's value length
  uint32_t CalcValueLength(const VectorDataValue &vctVal, ActionType type);

  // Save key and infor into buffer
  void FillHeaderBuff(RecStruct &recStru, uint32_t totalLen, uint32_t keyLen,
                      Byte verNum, uint64_t stamp, uint32_t valLen);

  void FillKeyBuff(RecStruct &recStru, const VectorDataValue &vctKey);

  // Save a version's value into buffer
  void FillValueBuff(ValueStruct &valStru, const VectorDataValue &vctVal);

  uint32_t CalcValidValueLength(RecStruct &recStru, bool bUpdate,
                                MVector<Byte> &vctSN);

protected:
  // If update this record, save old version for transaction rollback. If it
  // has been updated more than one time. old rec can recursively contain an
  // old rec.
  LeafRecord *_undoRec = nullptr;
  // Write statement, when insert, update or delete,
  Statement *_statement = nullptr;
  // Vector to contain overflow page
  OverflowPage *_overflowPage = nullptr;
  friend std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
  friend bool operator<(const LeafRecord &llr, const LeafRecord &rlr);
};

inline bool operator<(const LeafRecord &llr, const LeafRecord &rlr) {
  return llr < rlr;
}

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
class VectorLeafRecord : public MVector<LeafRecord *> {
public:
  using vector::vector;

  VectorLeafRecord(VectorLeafRecord &&src) noexcept { swap(src); }

  ~VectorLeafRecord() {
    for (auto iter = begin(); iter != end(); iter++) {
      (*iter)->DecRef();
    }
  }

  void RemoveAll() {
    for (auto iter = begin(); iter != end(); iter++) {
      (*iter)->DecRef();
    }

    clear();
  }

  VectorLeafRecord &operator=(VectorLeafRecord &&other) noexcept {
    RemoveAll();
    swap(other);
    return *this;
  }
};
} // namespace storage
