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
  RecordLock(ActionType t, RecordStatus s, bool gapLock, Statement *stmt,
             LeafPage *pPage = nullptr)
      : _actType(t), _status(s), _bGapLock(gapLock), _parentPage(pPage) {
    _vctStmt.push_back(stmt);
  }

  ActionType _actType;
  RecordStatus _status;
  bool _bGapLock; // True: locked the range between this and previous record,
                  // only valid repeatable read isolation level.
  MVector<Statement *> _vctStmt; // The statements locked this record, If write
                                 // lock, should have only one statement.
  LeafRecord *_undoRec{nullptr}; // To save old version for rollback statement,
                                 // only valid for parmary key.
  LeafPage *_parentPage;         // The parent leaf page
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
  // The start pointer to save values, multi stamp versions' values saved in the
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
 * @param arValStr The array to save ValueStruct, For SINGLE_VERSION, it is
 * always one element, or it will be a group of elements according to version
 * number.
 * @param fieldNum The number of fields in record value.
 * @param valVarLen The length of buffer to save variable fileds length (var
 * fields number * 4)
 * @param bFirst True: only read the first version; False: read all versions.
 */
void ReadValueStruct(RecStruct &recStru, ValueStruct *arValStr,
                     uint32_t fieldNum, uint32_t valVarLen, bool bFirst = true);

class LeafPage;
class Statement;
class LeafRecord : public RawRecord {
public:
  // Load LeafRecord from LeafPage
  LeafRecord(IndexType idxType, Byte *bys);
  // Constructor for secondary index LeafRecord
  LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey, Byte *bysPri,
             uint32_t lenPri, ActionType actType, Statement *stmt,
             uint64_t recStamp);
  // Constructor for primary index LeafRecord, only for insert
  LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey,
             const VectorDataValue &vctVal, uint64_t recStamp, Statement *stmt);
  LeafRecord(LeafRecord &&src)
      : RawRecord(src), _recLock(src._recLock),
        _overflowPage(src._overflowPage) {
    src._recLock = nullptr;
    src._overflowPage = nullptr;
  }
  LeafRecord(const LeafRecord &src) = delete;
  LeafRecord() : RawRecord() {}
  ~LeafRecord() {
    if (_recLock != nullptr) {
      assert(_recLock->_vctStmt.size() == 0);
      delete _recLock;
    }

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
  }
  LeafRecord &operator=(const LeafRecord &src) = delete;
  int32_t UpdateRecord(LeafPage *parentPage, const VectorDataValue &newVal,
                       uint64_t recStamp, Statement *stmt, ActionType type,
                       bool gapLock);

  ReadResult ReadListValue(const MHashMap<uint32_t, uint32_t> &mapPos,
                           VectorDataValue &vct, LeafPage *parentPage,
                           Statement *stmt = nullptr,
                           ActionType atype = ActionType::NO_ACTION) const;

  RawKey &GetKey(IndexTree *idxTree) const;
  /**Only for secondary index, Get the primary key, deep copy.*/
  RawKey &GetPrimayKey() const;

  int CompareTo(const LeafRecord &lr) const;
  int CompareKey(const RawKey &key) const;
  int CompareKey(const LeafRecord &lr) const;
  bool LoadOverflowPage(IndexTree *idxTree);

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
    assert(_recLock == nullptr || _recLock->_actType != ActionType::DELETE);
    uint16_t len = GetTotalLength();
    BytesCopy(bysPage, _bysVal, len);
    return len;
  }

  bool IsSole() const override {
    return _recLock == nullptr ? _bSole : _recLock->_undoRec->IsSole();
  }

  bool IsTransaction() const override {
    return _recLock != nullptr && _recLock->_vctStmt.size() > 0;
  }
  /**
   * @brief Lock current record, only valid for primary key
   * @param type Its value can only be QUERY_SHARE or QUERY_UPDATE
   * @param stmt The statement to lock this current
   * @param gapLock If lock it with gap lock
   * @return True: passed to lock; False: failed to lock
   */
  inline bool LockRecord(ActionType type, Statement *stmt, bool gapLock) {
    assert(type == ActionType::NO_ACTION || type == ActionType::QUERY_SHARE ||
           type == ActionType::QUERY_UPDATE);
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
  bool HasOverflowPage() {
    uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);
    return (*(_bysVal + UI16_2_LEN + keyLen) & REC_OVERFLOW) != 0;
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
  void SetParentPage(LeafPage *page) {
    assert(_recLock != nullptr);
    _recLock->_parentPage = page;
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
  uint32_t CalcValueLength(IndexTree *idxTree, const VectorDataValue &vctVal,
                           ActionType type);

  // Save key and infor into buffer
  void FillHeaderBuff(RecStruct &recStru, uint32_t totalLen, uint32_t keyLen,
                      Byte verNum, uint64_t stamp, uint32_t valLen,
                      ActionType type);

  void FillKeyBuff(RecStruct &recStru, const VectorDataValue &vctKey);

  // Save a version's value into buffer
  void FillValueBuff(ValueStruct &valStru, const VectorDataValue &vctVal);

  uint32_t CalcValidValueLength(IndexTree *idxTree, RecStruct &recStru,
                                bool bUpdate, MVector<Byte> &vctSN);

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
