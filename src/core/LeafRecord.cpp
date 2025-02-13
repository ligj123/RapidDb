#include "LeafRecord.h"
#include "../dataType/DataValueFactory.h"
#include "../pool/FilePagePool.h"
#include "../statement/Statement.h"
#include "../utils/ErrorID.h"
#include "../utils/ThreadPool.h"
#include "IndexTree.h"
#include "LeafPage.h"

#include <boost/crc.hpp>

namespace storage {
void ReadValueStruct(RecStruct &recStru, ValueStruct &valStr, uint32_t fieldNum,
                     uint32_t valVarLen) {
  uint32_t byNum = (fieldNum + 7) >> 3;
  Byte *bys = recStru._bysValStart;

  valStr.bysNull = bys;
  valStr.varFiledsLen = (uint32_t *)(bys + byNum);
  valStr.bysValue = bys + byNum + valVarLen;
}

// Initialize record struct
RecStruct::RecStruct(Byte *bys, uint16_t keyLen, OverflowPage *ofPage) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  _bysKey = bys + UI16_2_LEN;
  _byVerFlow = bys + UI16_2_LEN + keyLen;
  _arrStamp = (uint64_t *)(_byVerFlow + 1);
  _arrValLen = (uint32_t *)(((Byte *)_arrStamp) + UI64_LEN);

  if (ofPage != nullptr) [[unlikely]] {
    _arrCrc32 = (uint32_t *)(((Byte *)_arrValLen) + UI32_LEN);
    _pidStart = (PageID *)(((Byte *)_arrCrc32) + UI32_LEN);
    _pageNum = (uint16_t *)(((Byte *)_pidStart) + UI32_LEN);
    _bysValStart = ofPage->GetBysPage();
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart = ((Byte *)_arrValLen) + UI32_LEN;
  }
}

// Load record struct from byte array,
RecStruct::RecStruct(Byte *bys, OverflowPage *ofPage) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  uint16_t keyLen = *_keyLen;
  _bysKey = bys + UI16_2_LEN;
  _byVerFlow = bys + UI16_2_LEN + keyLen;
  _arrStamp = (uint64_t *)(_byVerFlow + 1);
  _arrValLen = (uint32_t *)(((Byte *)_arrStamp) + UI64_LEN);

  if ((*_byVerFlow) & REC_OVERFLOW) [[unlikely]] {
    _arrCrc32 = (uint32_t *)(((Byte *)_arrValLen) + UI32_LEN);
    _pidStart = (PageID *)(((Byte *)_arrCrc32) + UI32_LEN);
    _pageNum = (uint16_t *)(((Byte *)_pidStart) + UI32_LEN);
    if (ofPage != nullptr) {
      _bysValStart = ofPage->GetBysPage();
    } else {
      _bysValStart = nullptr;
    }
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart = ((Byte *)_arrValLen) + UI32_LEN;
  }
}

LeafRecord::LeafRecord(IndexType idxType, Byte *bys)
    : RawRecord(bys, false, idxType) {}

LeafRecord::LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey,
                       Byte *bysPri, uint32_t lenPri, ActionType actType,
                       uint64_t recStamp, Statement *stmt)
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->GetIndexType()) {
  uint16_t lenKey = CalcKeyLength(vctKey);
  if (stmt != nullptr) {
    _recLock = new RecordLock(actType, RecordStatus::INIT, false,
                              RecordResult::INIT, stmt->GetTxId(), stmt);
  }

  // The key is over length, failed to construct LeafRecord
  if (lenKey == UINT16_MAX) {
    if (_recLock == nullptr) {
      _threadErrorMsg.reset(
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)}));
    } else {
      _recLock->_errMsg =
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)});
    }

    _bValid = false;
    return;
  }

  int totalLen = lenKey + lenPri + UI16_2_LEN + UI64_LEN;
  _bysVal = CachePool::Apply(totalLen);
  Byte *bys = _bysVal;
  *((uint16_t *)bys) = totalLen;
  bys += UI16_LEN;
  *((uint16_t *)bys) = lenKey;
  bys += UI16_LEN;

  for (int i = 0; i < vctKey.size(); i++) {
    bys += vctKey[i]->WriteData(bys, SavePosition::KEY);
  }

  BytesCopy(bys, bysPri, lenPri);
  bys += lenPri;
  *((uint64_t *)bys) = recStamp;
}

LeafRecord::LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey,
                       const VectorDataValue &vctVal, uint64_t recStamp,
                       Statement *stmt)
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->GetIndexType()) {
  if (stmt != nullptr) {
    _recLock = new RecordLock(ActionType::INSERT, RecordStatus::INIT, false,
                              RecordResult::INIT, stmt->GetTxId(), stmt);
  }

  uint16_t lenKey = CalcKeyLength(vctKey);
  if (lenKey > Configure::GetMaxKeyLength()) {
    if (_recLock == nullptr) {
      _threadErrorMsg.reset(
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)}));
    } else {
      _recLock->_errMsg =
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)});
    }

    _bValid = false;
    return;
  }

  uint32_t lenVal = CalcValueLength(idxTree, vctVal, ActionType::INSERT);
  uint16_t infoLen = 1 + UI64_LEN + UI32_LEN;
  uint32_t max_lenVal =
      (uint32_t)Configure::GetMaxRecordLength() - lenKey - UI16_2_LEN - infoLen;

  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::INDEX_PAGE_SIZE - 1) / CachePage::INDEX_PAGE_SIZE;
    _overflowPage = idxTree->ApplyOvfPage(num);
    infoLen += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen =
      UI16_2_LEN + lenKey + infoLen +
      (_overflowPage == nullptr ? lenVal : UI32_LEN * 2 + UI16_LEN);

  _bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(_bysVal, lenKey, _overflowPage);
  FillHeaderBuff(recStru, totalLen, lenKey, 1, recStamp, lenVal,
                 ActionType::INSERT);
  FillKeyBuff(recStru, vctKey);

  ValueStruct valStru;
  ReadValueStruct(recStru, valStru, (uint32_t)vctVal.size(),
                  idxTree->GetValVarLen());

  FillValueBuff(valStru, vctVal);
  if (_overflowPage != nullptr) {
    (*recStru._pidStart) = _overflowPage->GetPageId();
    (*recStru._pageNum) = _overflowPage->GetPageNum();
    boost::crc_32_type crc32;
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();
  }
}

LeafRecord::LeafRecord(IndexTree *idxTree, const RawKey &priKey,
                       const VectorDataValue &vctVal, uint64_t recStamp,
                       Statement *stmt)
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->GetIndexType()) {
  if (stmt != nullptr) {
    _recLock = new RecordLock(ActionType::INSERT, RecordStatus::INIT, false,
                              RecordResult::INIT, stmt->GetTxId(), stmt);
  }

  uint16_t lenKey = priKey.GetLength();
  if (lenKey >= Configure::GetMaxKeyLength()) {
    if (_recLock == nullptr) {
      _threadErrorMsg.reset(
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)}));
    } else {
      _recLock->_errMsg =
          new ErrorMsg(CORE_EXCEED_KEY_LENGTH, {ToMString(lenKey)});
    }

    _bValid = false;
    return;
  }

  uint32_t lenVal = CalcValueLength(idxTree, vctVal, ActionType::INSERT);
  uint16_t infoLen = 1 + UI64_LEN + UI32_LEN;
  uint32_t max_lenVal =
      (uint32_t)Configure::GetMaxRecordLength() - lenKey - UI16_2_LEN - infoLen;

  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::INDEX_PAGE_SIZE - 1) / CachePage::INDEX_PAGE_SIZE;
    _overflowPage = idxTree->ApplyOvfPage(num);
    infoLen += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen =
      UI16_2_LEN + lenKey + infoLen +
      (_overflowPage == nullptr ? lenVal : UI32_LEN * 2 + UI16_LEN);

  _bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(_bysVal, lenKey, _overflowPage);
  FillHeaderBuff(recStru, totalLen, lenKey, 1, recStamp, lenVal,
                 ActionType::INSERT);
  BytesCopy(recStru._bysKey, priKey.GetBysVal(), lenKey);

  ValueStruct valStru;
  ReadValueStruct(recStru, valStru, (uint32_t)vctVal.size(),
                  idxTree->GetValVarLen());

  FillValueBuff(valStru, vctVal);
  if (_overflowPage != nullptr) {
    (*recStru._pidStart) = _overflowPage->GetPageId();
    (*recStru._pageNum) = _overflowPage->GetPageNum();
    boost::crc_32_type crc32;
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();
  }
}
/** @brief When update or delete this record, set new values into record and
 * save old value into _undoRec, only use for primary index
 * @param vctVal the vector of data value. If ActionType==Delete, it is empty
 * vector
 * @param recStamp the current record stamp.
 * @param tran the transaction
 * @param type only support Update or Delete
 * @return The updated record
 */
LeafRecord *LeafRecord::UpdateRecord(IndexTree *idxTree,
                                     const VectorDataValue &vctVal,
                                     uint64_t recStamp, Statement *stmt,
                                     ActionType type, bool gapLock) {
  assert(_indexType == IndexType::PRIMARY);
  assert(type == ActionType::UPDATE || type == ActionType::DELETE);
  assert(LockAble(stmt->GetTxId(), type));

  LeafRecord *lrNew = new LeafRecord();
  lrNew->_bSole = true;
  lrNew->_indexType = IndexType::PRIMARY;

  RecStruct recStruOld(_bysVal, _overflowPage);
  assert(recStruOld._pidStart == nullptr || _overflowPage != nullptr);
  if (_recLock != nullptr && _recLock->_bGapLock) {
    gapLock = true;
  }

  lrNew->_recLock =
      new RecordLock(type, RecordStatus::INIT, gapLock, RecordResult::IN_PAGE,
                     stmt->GetTxId(), stmt);
  lrNew->_recLock->_undoRec = this;

  uint32_t lenVal = CalcValueLength(idxTree, vctVal, type);
  uint32_t lenInfo = 1 + (UI64_LEN + UI32_LEN);
  uint32_t max_lenVal = (uint32_t)Configure::GetMaxRecordLength() -
                        *recStruOld._keyLen - UI16_2_LEN - lenInfo;
  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::INDEX_PAGE_SIZE - 1) / CachePage::INDEX_PAGE_SIZE;
    lrNew->_overflowPage = idxTree->ApplyOvfPage(num);
    lenInfo += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen = UI16_2_LEN + *recStruOld._keyLen + lenInfo +
                      (_overflowPage == nullptr ? lenVal : 0);

  lrNew->_bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(lrNew->_bysVal, *recStruOld._keyLen, lrNew->_overflowPage);
  FillHeaderBuff(recStru, totalLen, *recStruOld._keyLen, 1, recStamp, lenVal,
                 type);
  BytesCopy(recStru._bysKey, recStruOld._bysKey, *recStruOld._keyLen);
  if (type == ActionType::UPDATE) {
    ValueStruct valStru;
    ReadValueStruct(recStru, valStru, (uint32_t)vctVal.size(),
                    idxTree->GetValVarLen());
    FillValueBuff(valStru, vctVal);
  }

  if (lrNew->_overflowPage != nullptr) {
    (*recStru._pidStart) = lrNew->_overflowPage->GetPageId();
    (*recStru._pageNum) = lrNew->_overflowPage->GetPageNum();
    boost::crc_32_type crc32;
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();
  }

  return lrNew;
}

/**
 * @brief Read the value to data value list, only used for parmary index.
 * @param mapPos HashMap<fields positions, result position> in record that need
 * to read. if size=0, will read all fields. If field position=UINT32_MAX, means
 * to read Record Stamp.
 * @param vctVal The vector to save the data values.
 * @param stmt The statement to read list value.
 * @param atype ActionType
 * @return ReadResult
 */
ReadResult LeafRecord::ReadListValue(const MHashMap<uint32_t, uint32_t> &mapPos,
                                     VectorDataValue &vctVal,
                                     IndexTree *idxTree, Statement *stmt,
                                     ActionType atype, bool bGapLock) {
  assert(_indexType == IndexType::PRIMARY);
  assert(atype == ActionType::READ_UPDATE || atype == ActionType::READ_SHARE ||
         atype == ActionType::NO_ACTION);
  assert(_recLock == nullptr || !ReleaseLockAble());
  assert((stmt != nullptr && atype != ActionType::NO_ACTION) ||
         (stmt == nullptr && atype == ActionType::NO_ACTION));
  assert(vctVal.size() == 0);

  const LeafRecord *lr = this;

  if (_recLock != nullptr) {
    if (atype == ActionType::READ_UPDATE) {
      for (auto iter = _recLock->_lstTxid.begin();
           iter != _recLock->_lstTxid.end(); iter++) {
        if (*iter != TXID_NULL && *iter != stmt->GetTxId()) {
          return ReadResult::LOCKED;
        }
      }

      if (_recLock->_actType == ActionType::READ_SHARE) {
        _recLock->_actType = ActionType::READ_UPDATE;
        _recLock->_bGapLock = _recLock->_bGapLock | bGapLock;
        _recLock->_stmt = stmt;
      }
    } else if (atype == ActionType::READ_SHARE &&
               (_recLock->_actType != ActionType::READ_SHARE &&
                _recLock->_lstTxid.back() != stmt->GetTxId())) {
      return ReadResult::LOCKED;
    } else if (atype == ActionType::NO_ACTION &&
               (_recLock->_actType & ActionType::UPDATE_MASK) != 0) {
      while (true) {
        lr = lr->_recLock->_undoRec;
        if (lr == nullptr) {
          return ReadResult::LOCKED;
        } else if (lr->_recLock == nullptr) {
          break;
        }
      }
    }
  }

  if ((lr->_recLock != nullptr &&
       lr->_recLock->_actType == ActionType::DELETE)) {
    return ReadResult::REC_DELETE;
  }

  bool bAddLock = true;
  if (atype != ActionType::NO_ACTION) {
    assert(lr == this);
    if (_recLock == nullptr) {
      _recLock = new RecordLock(atype, RecordStatus::LOCK_ONLY, bGapLock,
                                RecordResult::IN_PAGE, stmt->GetTxId(), stmt);
    } else {
      for (auto txid : _recLock->_lstTxid) {
        if (txid == stmt->GetTxId()) {
          bAddLock = false;
        }
      }
      if (bAddLock) {
        _recLock->_lstTxid.push_back(stmt->GetTxId());
      }

      assert(_recLock->_actType == ActionType::READ_SHARE || !bAddLock);
    }
  } else {
    bAddLock = false;
  }

  RecStruct recStru(lr->_bysVal, lr->_overflowPage);
  assert(recStru._pidStart == nullptr || _overflowPage != nullptr);

  ValueStruct valStru;
  const VectorDataValue &vdSrc = idxTree->GetVctValue();
  ReadValueStruct(recStru, valStru, (uint32_t)vdSrc.size(),
                  idxTree->GetValVarLen());
  assert((*recStru._byVerFlow & REC_OVERFLOW) == 0 || _overflowPage != nullptr);

  int varField = -1;
  Byte *bys = valStru.bysValue;
  vctVal.resize(mapPos.size() > 0 ? mapPos.size() : vdSrc.size());

  for (size_t i = 0; i < vdSrc.size(); i++) {
    uint32_t flen = 0;
    if (!vdSrc[i]->IsFixLength()) {
      varField++;
      flen = valStru.varFiledsLen[varField];
    } else {
      flen = vdSrc[i]->GetMaxLength();
    }

    if (valStru.bysNull[i / 8] & (1 << i % 8)) {
      flen = 0;
    }

    if (mapPos.size() == 0) {
      IDataValue *dv = vdSrc[i]->Clone();
      if (flen > 0) {
        dv->ReadData(bys, flen, SavePosition::VALUE);
      }

      assert(vctVal[i] == nullptr);
      vctVal[i] = dv;
    } else {
      auto iter = mapPos.find(i);
      if (iter != mapPos.end()) {
        IDataValue *dv = vdSrc[i]->Clone();
        if (flen > 0) {
          dv->ReadData(bys, flen, SavePosition::VALUE);
        }

        assert(vctVal[iter->second] == nullptr);
        vctVal[iter->second] = dv;
      }
    }

    bys += flen;
  }

  auto iter = mapPos.find(UINT32_MAX);
  if (iter != mapPos.end()) {
    vctVal[iter->second] = new DataValueLong(recStru._arrStamp[0]);
  }

  return bAddLock ? ReadResult::OK_LOCK : ReadResult::OK_NOLOCK;
}

/**
 * @brief Get the value's length.
 * @return the value's length
 */
uint16_t LeafRecord::GetValueLength() const {
  if (_indexType == IndexType::PRIMARY) {
    return *(uint32_t *)(_bysVal + UI16_2_LEN + 1 +
                         (*(uint16_t *)(_bysVal + UI16_LEN)) + UI64_LEN);
  } else {
    return *(uint16_t *)_bysVal - *(uint16_t *)(_bysVal + UI16_LEN) -
           UI16_2_LEN - UI64_LEN;
  }
}

void LeafRecord::FillHeaderBuff(RecStruct &recStru, uint32_t totalLen,
                                uint32_t keyLen, Byte verNum, uint64_t stamp,
                                uint32_t valLen, ActionType type) {
  *recStru._totalLen = totalLen;
  *recStru._keyLen = keyLen;
#ifdef SINGLE_VERSION
  assert(verNum == 1);
#endif
  *recStru._byVerFlow = (recStru._pidStart == nullptr ? 0 : REC_OVERFLOW) +
                        (type == ActionType::DELETE ? REC_DELETE : 0) + verNum;
  recStru._arrStamp[0] = stamp;
  recStru._arrValLen[0] = valLen;
}

void LeafRecord::FillKeyBuff(RecStruct &recStru,
                             const VectorDataValue &vctKey) {
  Byte *bys = recStru._bysKey;

  for (size_t i = 0; i < vctKey.size(); i++) {
    uint16_t vl = vctKey[i]->WriteData(bys, SavePosition::KEY);
    bys += vl;
  }
}

void LeafRecord::FillValueBuff(ValueStruct &valStru,
                               const VectorDataValue &vctVal) {
  memset(valStru.bysNull, 0, (vctVal.size() + 7) >> 3);
  uint32_t *vlen = valStru.varFiledsLen;
  Byte *bys = valStru.bysValue;
  for (size_t i = 0; i < vctVal.size(); i++) {
    if (vctVal[i]->IsNull()) {
      valStru.bysNull[i / 8] |= 1 << (i % 8);
    }
    uint32_t vl = vctVal[i]->WriteData(bys, SavePosition::VALUE);
    bys += vl;
    if (!vctVal[i]->IsFixLength()) {
      *vlen = vl;
      vlen++;
    }
  }

  if (_overflowPage != nullptr) {
    _overflowPage->SetDirty();
  }
}

uint32_t LeafRecord::CalcValueLength(IndexTree *idxTree,
                                     const VectorDataValue &vctVal,
                                     ActionType type) {
  assert(_indexType == IndexType::PRIMARY);
  if (type == ActionType::DELETE)
    return 0;

  uint32_t lenVal = (uint32_t)(vctVal.size() + 7) / 8 + idxTree->GetValVarLen();
  for (size_t i = 0; i < vctVal.size(); i++) {
    lenVal += vctVal[i]->GetPersistenceLength(SavePosition::VALUE);
  }
  return lenVal;
}

/**
 * Load the overflow page
 *@param idxTree Index tree
 */
bool LeafRecord::LoadOverflowPage(IndexTree *idxTree, bool bsync) {
  uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);

  // To ensure this record has overflow page and it is nullptr
  assert((*(_bysVal + UI16_2_LEN + keyLen) & REC_OVERFLOW) != 0 &&
         _overflowPage == nullptr);

  Byte *bys = _bysVal + UI16_2_LEN + keyLen + 1 + UI64_LEN + UI32_LEN * 2;
  PageID pid = *(PageID *)(bys);
  uint16_t pnum = *(uint16_t *)(bys + UI32_LEN);
  _overflowPage = OverflowPage::GetPage(idxTree, pid, pnum, false);
  if (bsync) {
    bool b = FilePagePool::SyncReadPage(_overflowPage);
    assert(b);
    _overflowPage->SetPageStatus(PageStatus::VALID);
  } else {
    FilePagePool::AddReadPage(ThreadPool::GetThreadId(), _overflowPage);
  }
  return true;
}

/**
 * @brief Free _recLock and adjust records after commited, abort or rollback.
 * @param idxTree IndexTree
 * @return the variant length when abort or rollback
 */
ReleaseResult LeafRecord::ReleaseLock(IndexTree *idxTree) {
  assert(ReleaseLockAble());

  if (_recLock->_actType == ActionType::READ_SHARE ||
      _recLock->_actType == ActionType::READ_UPDATE) {
    assert(_recLock->_undoRec == nullptr);
    delete _recLock;
    _recLock = nullptr;
    return ReleaseResult::FINISHED;
  }

  assert(_recLock->_actType == ActionType::INSERT ||
         _recLock->_actType == ActionType::UPDATE ||
         _recLock->_actType == ActionType::DELETE);

  if (_recLock->GetRecordStatus() == RecordStatus::ROLLBACKED) {
    if (_overflowPage != nullptr) {
      idxTree->RecyclePageId(_overflowPage->GetPageId(),
                             _overflowPage->GetPageNum());
      delete _overflowPage;
      _overflowPage = nullptr;
    }

    if (_bSole && _bysVal != nullptr) {
      CachePool::Release(_bysVal, *((uint16_t *)_bysVal));
    }
    _bysVal = nullptr;
    _bSole = false;

    if (_recLock->_undoRec != nullptr) {
      LeafRecord *lr = _recLock->_undoRec;
      delete _recLock;
      *this = move(*lr);
      delete lr;
      if (_recLock == nullptr) {
        return ReleaseResult::FINISHED;
      } else if (!ReleaseLockAble()) {
        return ReleaseResult::UNFINISH;
      } else {
        return ReleaseLock(idxTree);
      }
    } else {
      delete _recLock;
      _recLock = nullptr;
      _bDelete = true;
      return ReleaseResult::DELETED;
    }
  }

  LeafRecord *lr = _recLock->_undoRec;
  bool bDel = (_recLock->_actType == ActionType::DELETE);
  delete _recLock;
  _recLock = nullptr;

  while (lr != nullptr) {
    assert(lr->IsStable());
    if (lr->_overflowPage != nullptr) {
      idxTree->RecyclePageId(lr->_overflowPage->GetPageId(),
                             lr->_overflowPage->GetPageNum());
      delete lr->_overflowPage;
      lr->_overflowPage = nullptr;
    }

    if (lr->_recLock == nullptr ||
        lr->_recLock->_actType == ActionType::READ_SHARE ||
        lr->_recLock->_actType == ActionType::READ_UPDATE) {
      if (lr->_recLock != nullptr) {
        delete lr->_recLock;
        lr->_recLock = nullptr;
      }

      delete lr;
      lr = nullptr;
    } else {
      LeafRecord *lr2 = lr->_recLock->_undoRec;
      delete lr->_recLock;
      lr->_recLock = nullptr;
      delete lr;
      lr = lr2;
    }
  }
  _bDelete = bDel;
  return bDel ? ReleaseResult::DELETED : ReleaseResult::FINISHED;
}

bool LeafRecord::ReleaseLockAble() const {
  if (_recLock == nullptr) {
    return false;
  }

  if (_recLock->_actType == ActionType::READ_SHARE) {
    for (auto iter = _recLock->_lstTxid.begin();
         iter != _recLock->_lstTxid.end(); iter++) {
      if (*iter != TXID_NULL)
        return false;
    }

    return true;
  } else {
    return _recLock->GetRecordStatus() >= RecordStatus::COMMITED;
  }
}

void LeafRecord::SubmitStatement(Statement &stmt, RecordStatus sts) {
  assert(_recLock != nullptr);
  assert(_recLock->GetRecordResult() != RecordResult::ERROR);
  assert(
      _recLock->GetRecordStatus() == RecordStatus::INIT &&
          (sts == RecordStatus::COMMITED || sts == RecordStatus::ROLLBACKED) ||
      _recLock->GetRecordStatus() == RecordStatus::LOCK_ONLY &&
          sts == RecordStatus::FREEED);

  if (sts == RecordStatus::FREEED &&
      _recLock->_actType == ActionType::READ_SHARE) {
    for (auto iter = _recLock->_lstTxid.begin();
         iter != _recLock->_lstTxid.end(); iter++) {
      if ((*iter) == stmt.GetTxId()) {
        (*iter) = TXID_NULL;
        return;
      }
    }
    assert(false);
  } else {
    assert(&stmt == _recLock->_stmt);
    _recLock->_stmt = nullptr;
    _recLock->_recStatus.store(sts, memory_order_release);
  }
}

MString LeafRecord::GetKeyString() {
  MString ss;
  ss.reserve(GetKeyLength() * 2 + 2);
  ss.append("0x");
  Byte *bys = _bysVal + UI16_2_LEN;
  for (uint32_t i = 0; i < GetKeyLength(); i++) {
    ss.append(HexStr[*bys]);
    bys++;
  }

  return ss;
}

void RawRecord::PrintKey(bool bchar) {
  if (bchar) {
    LOG_INFO << (char *)(_bysVal + UI16_2_LEN);
  } else {
    MString str;
    str.reserve(GetKeyLength() * 2 + 2);
    str.append("0x");
    Byte *bys = _bysVal + UI16_2_LEN;
    for (uint32_t i = 0; i < GetKeyLength(); i++) {
      str.append(HexStr[*bys]);
      bys++;
    }

    LOG_INFO << str;
  }
}

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr) {
  os << "TotalLen=" << lr.GetTotalLength() << "  Keys=";

  os << "0x";
  Byte *bys = lr._bysVal + UI16_2_LEN;
  for (uint32_t i = 0; i < lr.GetKeyLength(); i++) {
    os << HexStr[*bys];
    bys++;
  }

  return os;
}

} // namespace storage
