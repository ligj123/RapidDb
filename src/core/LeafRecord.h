#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/IDataValue.h"
#include "../transaction/TranStatus.h"
#include "../utils/BytesFuncs.h"
#include "../utils/ErrorMsg.h"
#include "IndexPage.h"
#include "OverflowPage.h"
#include "RawKey.h"
#include "RawRecord.h"

#include <cstring>

using namespace std;
namespace storage {
static const Byte REC_OVERFLOW = 0x80;
static const Byte VERSION_NUM = 0x0f;

enum class ActionType : uint8_t {
  QUERY_SHARE = 0,    // Query with read lock
  QUERY_UPDATE = 0x1, // Query with write lock
  INSERT = 0x10,      // Insert this record with write lock
  UPDATE = 0x11,      // Update this record with write lock
  DELETE = 0x12,      // Delete this record with write lock
  NO_ACTION = 0xFF    // No action to do for this record.
};

enum class RecordStatus : uint8_t {
  INIT = 0,  // Just create and wait to add LeafPage
  LOCK_ONLY, // Only lock current record, ActionType=QUERY_SHARE or QUERY_UPDATE
  INSERTED,  // The record has inserted its seat in LeafPage
  COMMIT,    // The related transaction has commited, only valid for WriteLock
  ROLLBACK   // The related statement has abort, only valid for WriteLock
};

class Statement;
class LeafRecord;

// LeafRecord lock
struct RecordLock {
  RecordLock(ActionType t, RecordStatus s, bool gapLock, Statement *stmt)
      : _actType(t), _status(s), _bGapLock(gapLock) {
    _vctStmt.push_back(stmt);
  }
  ActionType _actType;
  RecordStatus _status;
  bool _bGapLock; // True: locked the range between this and previous record,
                  // only valid repeatable read isolation level.
  MVector<Statement *> _vctStmt; // The statements locked this record, If write
                                 // lock, should have only one statement.
  LeafRecord *_undoRec{nullptr}; // To save old version for rollback statement.

  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
};

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
  // The byte to save the number of versions(low 4bits) and if it has overflow
  // page or not (the highest bit). For single version, only the highest bit is
  // valid.
  Byte *_byVerFlow;
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
  ~LeafRecord() {
    if (_recLock != nullptr)
      delete _recLock;

    if (_overflowPage != nullptr)
      delete _overflowPage;
  }

public:
  // Load LeafRecord from LeafPage
  LeafRecord(LeafPage *indexPage, Byte *bys);
  // No use now, only for test
  LeafRecord(IndexTree *indexTree, Byte *bys);
  LeafRecord(LeafRecord &&src);
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType type, Statement *stmt,
             uint64_t recStamp);
  // Constructor for primary index LeafRecord, only for insert
  LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp, Statement *stmt);

  int32_t UpdateRecord(const VectorDataValue &newVal, uint64_t recStamp,
                       Statement *stmt, ActionType type, bool gapLock);

  inline int GetListValue(VectorDataValue &vct, uint64_t verStamp = UINT64_MAX,
                          Statement *stmt = nullptr, bool bQuery = true) const {
    return GetListValue({}, vct, verStamp, stmt, bQuery);
  }
  int GetListValue(const MVector<int> &vctPos, VectorDataValue &vct,
                   uint64_t verStamp = UINT64_MAX, Statement *stmt = nullptr,
                   bool bQuery = true) const;
  RawKey *GetKey() const;
  /**Only for secondary index, Get the primary key, deep copy.*/
  RawKey *GetPrimayKey() const;

  int CompareTo(const LeafRecord &lr) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const LeafRecord &lr) const;

  /**Only the bytes' length in IndexPage, key length + value length without
   * overflow page content*/
  uint16_t GetTotalLength() const override {
    return (_recLock == nullptr || _recLock->_actType != ActionType::DELETE)
               ? *((uint16_t *)_bysVal)
               : 0;
  }
  // Get the value's length. If there have more than one version, return the
  // first version's length.
  uint16_t GetValueLength() const override;

  inline uint16_t SaveData(Byte *bysPage) {
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  bool IsSole() const override {
    return _recLock == nullptr ? _bSole : _recLock->_undoRec->IsSole();
  }

  bool IsTransaction() const override { return _recLock != nullptr; }
  /**
   * @brief Lock current record, only valid for primary key
   * @param type Its value can only be QUERY_SHARE or QUERY_UPDATE
   * @param stmt The statement to lock this current
   * @param gapLock If lock it with gap lock
   * @return True: passed to lock; False: failed to lock
   */
  inline bool LockRecord(ActionType type, Statement *stmt, bool gapLock) {
    assert(type == ActionType::QUERY_SHARE || type == ActionType::QUERY_UPDATE);
    if (_recLock != nullptr) {
      if (type == ActionType::QUERY_UPDATE)
        return false;
      if (_recLock->_actType != ActionType::QUERY_SHARE)
        return false;

      _recLock->_vctStmt.push_back(stmt);
    } else {
      _recLock = new RecordLock(type, RecordStatus::LOCK_ONLY, gapLock, stmt);
    }
  }

  bool IsTransaction() { return _recLock != nullptr; }
  bool IsGapLock() { return _recLock != nullptr && _recLock->_bGapLock; }
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
  ActionType GetAction() {
    return _recLock == nullptr ? ActionType::NO_ACTION : _recLock->_actType;
  }
  void ReleaseOverflowPageID();

  inline IndexTree *GetTreeFile() const {
    return _bInPage ? _parentPage->GetIndexTree() : _indexTree;
  }

protected:
  // To calc key length
  inline uint32_t CalcKeyLength(const VectorDataValue &vctKey) {
    uint32_t lenKey = 0;
    for (int i = 0; i < vctKey.size(); i++) {
      lenKey += vctKey[i]->GetPersistenceLength(SavePosition::KEY);
    }

    if (lenKey > Configure::GetMaxKeyLength()) {
      _threadErrorMsg.reset(
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)}));
      return UINT32_MAX;
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
  // Default is nullptr, If a statement locked this record, set this variable to
  // save related information
  RecordLock *_recLock{nullptr};
  // Vector to contain overflow page
  OverflowPage *_overflowPage{nullptr};
  friend std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
};

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
using VectorLeafRecord = MVector<LeafRecord *>;
} // namespace storage
