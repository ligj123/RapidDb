#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/IDataValue.h"
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
static const Byte REC_DELETE = 0x40;
static const Byte VERSION_NUM = 0x0f;

class Statement;
class LeafRecord;
class LeafPage;

// LeafRecord lock
struct RecordLock {
  RecordLock(ActionType t, RecordStatus s, bool gapLock, RecordResult recRst,
             TranID txid, Statement *stmt = nullptr,
             LeafRecord *undoRec = nullptr)
      : _actType(t), _recStatus(s), _bGapLock(gapLock), _recResult(recRst),
        _undoRec(undoRec) {
    if (t == ActionType::READ_SHARE && !_bGapLock) {
      _stmt = nullptr;
    } else {
      _stmt = stmt;
    }

    _lstTxid.push_back(txid);
  }

  ~RecordLock() {
    if (_errMsg != nullptr) {
      delete _errMsg;
      _errMsg = nullptr;
    }

    assert(_undoRec == nullptr);
  }
  // Get transaction id, if more than 1, return the first txid
  uint64_t TxID() {
    assert(_lstTxid.size() > 0);
    return *_lstTxid.begin();
  }

  RecordStatus GetRecordStatus(bool acquire = false) {
    if (acquire) {
      return _recStatus.load(memory_order_acquire);
    } else {
      return _recStatus.load(memory_order_relaxed);
    }
  }
  RecordResult GetRecordResult(bool acquire = false) {
    if (acquire) {
      return _recResult.load(memory_order_acquire);
    } else {
      return _recResult.load(memory_order_relaxed);
    }
  }

  ActionType _actType;
  atomic<RecordStatus> _recStatus;
  // True: locked the range between this and previous record, only valid
  // repeatable read isolation level. In this version, a record can only has a
  // gap lock.
  bool _bGapLock;
  // The operated result of this record
  atomic<RecordResult> _recResult;
  // The statement that inserted, updated or deleted the record. If
  // ActionType::READ_SHARE and _bGapLock=FALSE , it should be nullptr.
  Statement *_stmt;
  // The transaction id locked this record, If write lock, should have only one
  // transaction id. If ActionType=READ_SHARE, It need to set the related txid
  // to TXID_NULL, when all txid=TXID_NULL, this lock can be releases, else it
  // should update _recStatus and set txid=TXID_NULL at the same time.
  MList<uint64_t> _lstTxid;
  // To save old version for rollback statement, only valid for primary key.
  LeafRecord *_undoRec;
  // To save error message that happened in operation
  ErrorMsg *_errMsg{nullptr};

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
};

// Only for primary index
struct RecStruct {
  // Initialize record struct
  RecStruct(Byte *bys, uint16_t keyLen, OverflowPage *ofPage);
  // Load record struct from byte array,
  RecStruct(Byte *bys, OverflowPage *ofPage);

  // The pointer to save total length for record, not include values in overflow
  // page length, only to calc the occupied bytes in LeafPage buffer
  uint16_t *_totalLen;
  // The pointer to save key length, only include key content length.
  uint16_t *_keyLen;
  // The pointer to save key content in buffer
  Byte *_bysKey;
  // The pointer to one byte,
  // The highest bit is if this record has overflow page;
  // The second bit is if this record has been delete;
  // The low 4 bits save the number of stamp versions;
  // Other 2 bits are reserved.
  // For SINGLE_VERSION, only the high 2 bits are valid and always one stamp
  // version.
  Byte *_byVerFlow;
  // The pointer array to save stamps version, 1~N elements.
  uint64_t *_arrStamp;
  // The pointer array to save length for every version's value.
  uint32_t *_arrValLen;
  // The address to save the crc32, only valid when it has overflow page.
  uint32_t *_arrCrc32;
  // The start page id, only valid when it has overflow page.
  PageID *_pidStart;
  // The page number, only valid when it has overflow page.
  uint16_t *_pageNum;
  // The start pointer to save values, multi stamp versions' values saved in
  // reverse order.
  Byte *_bysValStart;
};

struct ValueStruct {
  // (n+7)/8 bytes to save that the related fields are null or not, every field
  // occupys a bit. n: fields number.
  Byte *bysNull;
  // The postion to save variable length fields' lengths in buffer, only
  // variable length fields, do not need to consider other type fields.
  uint32_t *varFiledsLen;
  // Start position to save value content
  Byte *bysValue;
};

/**
 * @brief Read ValueStruct from buffer
 * @param recStru Record Struct
 * @param valStr The ValueStruct to read, Now it is only SINGLE_VERSION. In the
 * future maybe support multi versions.
 * @param fieldNum The number of fields in record value.
 * @param valVarLen The length of buffer to save variable fileds length (var
 * fields number * 4)
 */
void ReadValueStruct(RecStruct &recStru, ValueStruct &valStr, uint32_t fieldNum,
                     uint32_t valVarLen);

class LeafPage;
class Statement;
class LeafRecord : public RawRecord {
public:
  // Load LeafRecord from LeafPage
  LeafRecord(IndexType idxType, Byte *bys);
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType actType, uint64_t recStamp,
             Statement *stmt = nullptr);
  // Constructor for primary index LeafRecord, only for insert
  LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp,
             Statement *stmt = nullptr);
  LeafRecord(IndexTree *idxTree, const RawKey &priKey,
             const VectorDataValue &vctVal, uint64_t recStamp,
             Statement *stmt = nullptr);
  LeafRecord(LeafRecord &&src)
      : RawRecord(move(src)), _recLock(src._recLock),
        _overflowPage(src._overflowPage) {
    src._recLock = nullptr;
    src._overflowPage = nullptr;
  }
  LeafRecord(const LeafRecord &src) = delete;
  LeafRecord() : RawRecord() {}
  ~LeafRecord() {
    assert(_recLock == nullptr);

    if (_overflowPage != nullptr)
      delete _overflowPage;
  }

  LeafRecord &operator=(LeafRecord &&src) {
    _bysVal = src._bysVal;
    src._bysVal = nullptr;
    _bSole = src._bSole;
    _indexType = src._indexType;
    _recLock = src._recLock;
    _overflowPage = src._overflowPage;
    src._recLock = nullptr;
    src._overflowPage = nullptr;
    return *this;
  }
  LeafRecord &operator=(const LeafRecord &src) = delete;

  LeafRecord *UpdateRecord(IndexTree *idxTree, const VectorDataValue &newVal,
                           uint64_t recStamp, Statement *stmt, ActionType type,
                           bool gapLock);

  ReadResult ReadListValue(const MHashMap<uint32_t, uint32_t> &mapPos,
                           VectorDataValue &vct, IndexTree *idxTree,
                           Statement *stmt = nullptr,
                           ActionType atype = ActionType::NO_ACTION,
                           bool bGapLock = false);
  bool LoadOverflowPage(IndexTree *idxTree, bool bsync = false);
  ReleaseResult ReleaseLock(IndexTree *idxTree);
  uint16_t GetValueLength() const override;
  uint16_t GetDataLength() const override {
    assert(_indexType == IndexType::NON_UNIQUE);
    return (uint16_t)(*((uint16_t *)_bysVal) - UI16_2_LEN - UI64_LEN);
  }
  /**
   * @brief To judge if the lock can be released
   */
  bool ReleaseLockAble() const;

  void SubmitStatement(Statement &stmt, RecordStatus s);

  MString GetKeyString();

  /**
   * @brief Get key from record, deep copy
   */
  inline RawKey GetKey() const {
    return RawKey(GetKeyLength(), _bysVal + UI16_2_LEN);
  }

  /**
   * @brief Only for secondary index, Get the primary key, deep copy.*/
  inline RawKey GetPrimayKey() const {
    int start = GetKeyLength() + UI16_2_LEN;
    int len = GetTotalLength() - start - UI64_LEN;
    return RawKey(len, _bysVal + start);
  }

  inline int CompareTo(const LeafRecord &lr) const {
    return BytesCompare(
        _bysVal + UI16_2_LEN, GetTotalLength() - UI16_2_LEN - UI64_LEN,
        lr._bysVal + UI16_2_LEN, lr.GetTotalLength() - UI16_2_LEN - UI64_LEN);
  }

  inline int CompareKey(const RawKey &key) const {
    return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength(), key.GetBysVal(),
                        key.GetLength());
  }

  inline int CompareKey(const LeafRecord &lr) const {
    return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength(),
                        lr.GetBysValue() + UI16_2_LEN, lr.GetKeyLength());
  }

  /**Only the bytes' length in IndexPage, key length + value length without
   * overflow page content*/
  inline uint16_t GetTotalLength() const override {
    if (_bDelete ||
        _recLock != nullptr && _recLock->_actType == ActionType::DELETE) {
      return 0;
    } else {
      return *((uint16_t *)_bysVal);
    }
  }

  uint16_t GetActualLength() { return *((uint16_t *)_bysVal); }

  inline uint16_t SaveData(Byte *bysPage) {
    assert(_recLock == nullptr);
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  /**-
   * @brief True: The record has committed or abort and can be visit and saved
   * into disk. False: The record is uncommitted.
   */
  inline bool IsStable() {
    if (_recLock == nullptr ||
        (_recLock->_actType & ActionType::UPDATE_MASK) == 0) {
      return true;
    } else if (_recLock->GetRecordStatus() >= RecordStatus::COMMITED) {
      return true;
    }

    return false;
  }

  inline bool IsGapLock() { return _recLock != nullptr && _recLock->_bGapLock; }
  inline bool HasOverflowPage() {
    uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);
    return (*(_bysVal + UI16_2_LEN + keyLen) & REC_OVERFLOW) != 0;
  }

  inline ActionType GetAction() {
    return _recLock == nullptr ? ActionType::NO_ACTION : _recLock->_actType;
  }

  // To calc key length
  inline static uint16_t CalcKeyLength(const VectorDataValue &vctKey) {
    uint32_t lenKey = 0;
    for (int i = 0; i < vctKey.size(); i++) {
      lenKey += vctKey[i]->GetPersistenceLength(SavePosition::KEY);
    }

    if (lenKey > Configure::GetMaxKeyLength()) {
      return UINT16_MAX;
    }

    return static_cast<uint16_t>(lenKey);
  }

  OverflowPage *GetOverflowPage() { return _overflowPage; }
  RecordLock *GetLock() { return _recLock; }
  bool IsConflict(TranID txid, ActionType atype) {
    if (_recLock == nullptr ||
        _recLock->GetRecordStatus() >= RecordStatus::COMMITED) {
      return false;
    }

    if (atype == ActionType::READ_SHARE &&
        _recLock->_actType == ActionType::READ_SHARE) {
      return false;
    }

    for (TranID id : _recLock->_lstTxid) {
      if (id != txid && id != TXID_NULL) {
        return true;
      }
    }

    return false;
  }
  void GetLength(int32_t &tempLen, int32_t &commitLen) {
    tempLen = GetTotalLength();
    LeafRecord *lr = this;
    while (lr->_recLock != nullptr && lr->_recLock->_undoRec != nullptr) {
      lr = lr->_recLock->_undoRec;
    }

    if (lr->IsStable()) {
      commitLen = lr->GetTotalLength();
    } else {
      commitLen = 0;
    }
  }
  size_t Hash() const {
    return BytesHash(_bysVal + UI16_2_LEN, GetKeyLength());
  }

  bool LockAble(TranID txid, ActionType actType) {
    if (_recLock == nullptr) {
      return true;
    } else if (_recLock->_lstTxid.size() == 1 &&
               _recLock->_lstTxid.back() == txid) {
      return true;
    } else if (actType == READ_SHARE && _recLock->_actType == READ_SHARE) {
      return true;
    }

    return false;
  }

protected:
  // To calc a version's value length
  uint32_t CalcValueLength(IndexTree *idxTree, const VectorDataValue &vctVal,
                           ActionType type);

  // Save key and infor into buffer
  void FillHeaderBuff(RecStruct &recStru, uint32_t totalLen, uint32_t keyLen,
                      Byte verNum, uint64_t stamp, uint32_t valLen,
                      ActionType type);

  void FillKeyBuff(RecStruct &recStru, const VectorDataValue &vctKey);

  // Save a version's value into buffer
  void FillValueBuff(ValueStruct &valStru, const VectorDataValue &vctVal);

protected:
  // Default is nullptr, If a statement locked this record, set this variable to
  // save related information
  RecordLock *_recLock{nullptr};
  // Vector to contain overflow page
  OverflowPage *_overflowPage{nullptr};
  friend std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
  friend class LeafPage;
};

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr);
using VectorLeafRecord = MVector<LeafRecord *>;

struct LeafRecordHash {
  size_t operator()(const LeafRecord *pLr) const { return pLr->Hash(); }
};

struct LeafRecordEqual {
  bool operator()(const LeafRecord *lLr, const LeafRecord *rLr) const {
    return BytesEqual(lLr->GetBysValue() + UI16_2_LEN, lLr->GetKeyLength(),
                      rLr->GetBysValue() + UI16_2_LEN, rLr->GetKeyLength());
  }
};

struct LeafRecordCmp {
  bool operator()(const LeafRecord *lLr, const LeafRecord *rLr) const {
    return BytesCompare(lLr->GetBysValue() + UI16_2_LEN, lLr->GetKeyLength(),
                        rLr->GetBysValue() + UI16_2_LEN, rLr->GetKeyLength());
  }
};
} // namespace storage
