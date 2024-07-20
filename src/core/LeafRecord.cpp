#include "LeafRecord.h"
#include "../dataType/DataValueFactory.h"
#include "../pool/StoragePool.h"
#include "../statement/Statement.h"
#include "../utils/ErrorID.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include "pool/FilePagePool.h"
#include "utils/ThreadPool.h"

#include <boost/crc.hpp>

namespace storage {
static thread_local boost::crc_32_type crc32;

void ReadValueStruct(RecStruct &recStru, ValueStruct *arValStr,
                     uint32_t fieldNum, uint32_t valVarLen, bool bFirstOnly) {
  uint32_t byNum = (fieldNum + 7) >> 3;
  uint32_t vnum = bFirstOnly ? 1 : (*recStru->_byVerFlow & VERSION_NUM);
  Byte *bys = recStru._bysValStart;

  uint32_t offset = 0;
  for (Byte i = 0; i < verNum; i++) {
    arvalStr[i].bysNull = bys + offset;
    arvalStr[i].varFiledsLen = (uint32_t *)(bys + offset + byNum);
    arvalStr[i].bysValue = bys + offset + byNum + valVarLen;
    offset += recStru._arrValLen[i];
  }
}

// Initialize record struct
RecStruct::RecStruct(Byte *bys, uint16_t keyLen, OverflowPage *ofPage) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  _bysKey = bys + UI16_2_LEN;
  _byVerFlow = bys + UI16_2_LEN + keyLen;
  _arrStamp = (uint64_t *)(_byVerFlow + 1);
  _arrValLen = (uint32_t *)(((Byte *)_arrStamp) + UI64_LEN);

  if ((*_byVerFlow) & REC_OVERFLOW) [[unlikely]] {
    _arrCrc32 = (uint32_t *)(((Byte *)_arrValLen) + UI32_LEN);
    _pidStart = (PageID *)(((Byte *)_arrCrc32) + UI32_LEN);
    _pageNum = (uint16_t *)(((Byte *)_pidStart) + UI32_LEN);
    assert(ofPage != nullptr);
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
                       Statement *stmt, uint64_t recStamp)
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->ReadIndexType()) {
  uint16_t lenKey = CalcKeyLength(vctKey);
  // The key is over length, failed to construct LeafRecord
  if (lenKey == UINT16_MAX) {
    _bValid = false;
    return;
  }

  _recLock = new RecordLock(ActionType::INSERT, RecordStatus::INIT, false,
                            false, stmt->GetTxId(), stmt->GetId());
  int totalLen = lenKey + lenPri + UI16_2_LEN + UI64_LEN;
  _bysVal = CachePool::Apply(totalLen);
  Byte *bys = _bysVal;
  *((uint16_t *)bys) = totalLen;
  bys += UI16_LEN;
  *((uint16_t *)bys) = lenKey;

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
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->ReadIndexType()) {
  uint16_t lenKey = CalcKeyLength(vctKey);
  if (lenKey == UINT16_MAX) {
    _bValid = false;
    return;
  }

  _recLock = new RecordLock(ActionType::INSERT, RecordStatus::INIT, false,
                            false, stmt->GetTxId(), stmt->GetId());
  uint32_t lenVal = CalcValueLength(idxTree, vctVal, ActionType::INSERT);
  uint16_t infoLen = 1 + UI64_LEN + UI32_LEN;
  uint32_t max_lenVal =
      (uint32_t)Configure::GetMaxRecordLength() - lenKey - UI16_2_LEN - infoLen;

  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::INDEX_PAGE_SIZE - 1) / CachePage::INDEX_PAGE_SIZE;
    _overflowPage =
        OverflowPage::GetPage(idxTree, idxTree->ApplyPageId(num), num, true);
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
  ReadValueStruct(recStru, &valStru, (uint32_t)vctVal.size(),
                  idxTree->GetValVarLen(), true);

  FillValueBuff(valStru, vctVal);
  if (_overflowPage != nullptr) {
    crc32.reset();
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();

    FilePagePool::AddWritePage(ThreadPool::GetThreadId(), _overflowPage);
  }
}

/** @brief When update or delete this record, set new values into record and
 * save old value into _undoRec, only use for primary index
 * @param vctVal the vector of data value. If ActionType==Delete, it is empty
 * vector
 * @param recStamp the current record stamp.
 * @param tran the transaction
 * @param type only support Update or Delete
 * @return different of the bytes occpied
 */
int32_t LeafRecord::UpdateRecord(LeafPage *parentPage,
                                 const VectorDataValue &vctVal,
                                 uint64_t recStamp, Statement *stmt,
                                 ActionType type, bool gapLock) {
  assert(type == ActionType::UPDATE || type == ActionType::DELETE);
  RecStruct recStruOld(_bysVal, _overflowPage);
  assert(recStruOld._pidStart == nullptr || _overflowPage != nullptr);
  RecordLock *recLock =
      new RecordLock(type, RecordStatus::INIT, gapLock, stmt, parentPage);
  recLock->_undoRec = new LeafRecord(move(*this));
  _recLock = recLock;
  IndexTree *idxTree = parentPage->GetIndexTree();

  uint32_t lenVal = CalcValueLength(idxTree, vctVal, type);
  uint32_t lenInfo = 1 + (UI64_LEN + UI32_LEN);
  uint32_t max_lenVal = (uint32_t)Configure::GetMaxRecordLength() -
                        *recStruOld._keyLen - UI16_2_LEN - lenInfo;
  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::INDEX_PAGE_SIZE - 1) / CachePage::INDEX_PAGE_SIZE;
    _overflowPage =
        OverflowPage::GetPage(idxTree, idxTree->ApplyPageId(num), num, true);
    lenInfo += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen = UI16_2_LEN + *recStruOld._keyLen + lenInfo +
                      (_overflowPage == nullptr ? lenVal : 0);

  _bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(_bysVal, *recStruOld._keyLen, _overflowPage);
  FillHeaderBuff(recStru, totalLen, *recStruOld._keyLen, 1, recStamp, lenVal,
                 type);
  BytesCopy(recStru._bysKey, recStruOld._bysKey, *recStruOld._keyLen);
  if (type == ActionType::UPDATE) {
    ValueStruct valStru;
    ReadValueStruct(recStru, &valStru, (uint32_t)vctVal.size(),
                    idxTree->GetValVarLen());
    FillValueBuff(valStru, vctVal);
  }

  if (_overflowPage != nullptr) {
    crc32.reset();
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();
  }

  return *recStru._totalLen - *recStruOld._totalLen;
}

/**
 * @brief To calc the value's length that will copy to new record. It will
 * exclude repeated version.<Only for multi versions>
 * @param idxTree IndexTree
 * @param recStru
 * @param bUpdate True: update this record and will judge the last version if
 * repeate with new version. False: only remove repeated version
 * @param vctSN To save the serial number that will moved to new record
 * @return The total length of versions will been moved new record
 */
uint32_t LeafRecord::CalcValidValueLength(IndexTree *idxTree,
                                          RecStruct &recStru, bool bUpdate,
                                          MVector<Byte> &vctSN) {
  const MTreeSet<VersionStamp, KeyCmp> &setVer =
      idxTree->GetHeadPage()->GetSetVerStamp();
  if (setVer.size() == 0)
    return 0;

  uint32_t len = 0;
  Byte verNum = (*recStru._byVerFlow) & 0x0f;

  auto iter = setVer.begin();
  if (!bUpdate || *iter > recStru._arrStamp[0]) {
    len += recStru._arrValLen[0];
    vctSN.push_back(0);
  }
  iter++;

  for (Byte i = 1; i < verNum; i++) {
    if (*iter <= recStru._arrStamp[i]) {
      continue;
    }

    len += recStru._arrValLen[i];
    vctSN.push_back(i);

    while (*iter > recStru._arrStamp[i]) {
      iter++;
      if (iter == setVer.end())
        break;
    }

    if (iter == setVer.end())
      break;
  }

  return len;
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
                                     LeafPage *parentPage, Statement *stmt,
                                     ActionType atype) const {
  assert(_indexType == IndexType::PRIMARY);
  const LeafRecord *lr = this;
  IndexTree *idxTree = parentPage->GetIndexTree();
  if (_recLock != nullptr) {
    if (atype & ActionType::WRITE_LOCK_MASK)
      return ReadResult::LOCKED;
    if (atype == ActionType::READ_SHARE &&
        _recLock->_actType & ActionType::WRITE_LOCK_MASK)
      return ReadResult::LOCKED;
  }

  if (atype == ActionType::NO_ACTION) {
    while (lr->_recLock->_actType & ActionType::UPDATE_MASK) {
      lr = lr->_recLock->_undoRec;
      if (lr == nullptr)
        return ReadResult::LOCKED;
    }
  }

  RecStruct recStru(lr->_bysVal, lr->_overflowPage);
  ValueStruct valStru;
  const VectorDataValue &vdSrc = idxTree->GetVctValue();
  ReadValueStruct(recStru, &valStru, (uint32_t)vdSrc.size(),
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
    uint32_t pos;
    if (mapPos.size() == 0) {
      pos = i;
    } else {
      auto iter = mapPos.find(i);
      pos = (iter == mapPos.end() ? UINT32_MAX : iter->second);
    }

    if (mapPos.size() == 0 || mapPos.find(i) != mapPos.end()) {
      IDataValue *dv = vdSrc[i]->Clone();
      if (flen > 0)
        dv->ReadData(bys, flen, SavePosition::VALUE);

      vctVal[pos] = dv;
    }

    bys += flen;
  }

  return ReadResult::OK;
}

RawKey LeafRecord::GetKey(IndexTree *idxTree) const {
  return RawKey(GetKeyLength(), _bysVal + UI16_2_LEN);
}

RawKey *LeafRecord::GetPrimayKey() const {
  int start = GetKeyLength() + UI16_2_LEN;
  int len = GetTotalLength() - start - UI64_LEN;
  return new RawKey(len, _bysVal + start);
}

int LeafRecord::CompareTo(const LeafRecord &lr) const {
  return BytesCompare(_bysVal + UI16_2_LEN, GetTotalLength() - UI16_2_LEN,
                      lr._bysVal + UI16_2_LEN,
                      lr.GetTotalLength() - UI16_2_LEN);
}

int LeafRecord::CompareKey(const RawKey &key) const {
  return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength() - UI16_2_LEN,
                      key.GetBysVal(), key.GetLength());
}

int LeafRecord::CompareKey(const LeafRecord &lr) const {
  return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength() - UI16_2_LEN,
                      lr.GetBysValue() + UI16_2_LEN,
                      lr.GetKeyLength() - UI16_2_LEN);
}

/**
 * @brief Get the value's length. If there have more than one version, return
 * the newest version's length.
 * @return the value's length
 */
uint16_t LeafRecord::GetValueLength() const {
  if (_indexType == IndexType::PRIMARY) {
#ifdef SINGLE_VERSION
    return *(uint32_t *)(_bysVal + UI16_2_LEN + 1 +
                         (*(uint16_t *)(_bysVal + UI16_LEN)) + UI64_LEN);
#else
    Byte vNum = GetVersionNumber();
    return *(uint32_t *)(_bysVal + UI16_2_LEN + 1 +
                         (*(uint16_t *)(_bysVal + UI16_LEN)) + UI64_LEN * vNum);
#endif
  } else {
    return *(uint16_t *)_bysVal - *(uint16_t *)(_bysVal + UI16_LEN) -
           UI16_2_LEN;
  }
}

bool LeafRecord::IsInTransaction() const {
  if (_recLock == nullptr)
    return false;
  if (_recLock->_actType == ActionType::READ_SHARE) {
    if (_recLock->_status == RecordStatus::FREEED)
      return false;
    for (auto iter = _recLock->_lstTxid.begin();
         iter != _recLock->_lstTxid.end() iter++) {
      if (*iter != TXID_NULL)
        return true;
    }

    _recLock->_status = RecordStatus::FREEED;
    return false;
  } else {
    return _recLock->_status < RecordStatus::COMMITED;
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
bool LeafRecord::LoadOverflowPage(IndexTree *idxTree) {
  uint16_t keyLen = *(uint16_t *)(_bysVal + UI16_LEN);

  // To ensure this record has overflow page and it is nullptr
  assert((*(_bysVal + UI16_2_LEN + keyLen) & REC_OVERFLOW) != 0 &&
         _overflowPage == nullptr);

#ifdef SINGLE_VERSION
  Byte *bys = _bysVal + UI16_2_LEN + keyLen + 1 + UI64_LEN + UI32_LEN * 2;
#else
  Byte flag = *(_bysVal + UI16_2_LEN + keyLen);
  Byte ver = flag & VERSION_NUM;
  Byte *bys =
      _bysVal + UI16_2_LEN + keyLen + 1 + UI64_LEN * ver + UI32_LEN * 2 * ver;
#endif
  PageID pid = *(PageID *)(bys);
  uint16_t pnum = *(uint16_t *)(bys + UI32_LEN);
  _overflowPage = OverflowPage::GetPage(idxTree, pid, pnum, false);
  return true;
}

/**
 * @brief Free _recLock and adjust records when commited, abort or rollback.
 * @param pPage parent page for the record
 * @return the variant length when abort or rollback
 */
int LeafRecord::ReleaseLock(LeafPage *pPage) {
  assert(ReleaseLockAble());
  if (_recLock->_actType == ActionType::READ_SHARE ||
      _recLock->_actType == ActionType::READ_UPDATE) {
    assert(_recLock->_undoRec == nullptr);
    delete _recLock;
    _recLock = nullptr;
    return 0;
  } else {
    assert(_recLock->_actType == ActionType::INSERT ||
           _recLock->_actType == ActionType::UPDATE ||
           _recLock->_actType == ActionType::DELETE);

    LeafRecord *lr = _recLock->_undoRec;
    if (_recLock->_status == RecordStatus::ABORT ||
        _recLock->_status == RecordStatus::ROLLBACK) {
      if (_bSole && _bysVal != nullptr) {
        CachePool::Release(_bysVal, *((uint16_t *)_bysVal));
      }

      if (_overflowPage != nullptr) {
        pPage->GetIndexTree()->RecyclePageId(_overflowPage->GetPageId(),
                                             _overflowPage->GetPageNum());
        delete _overflowPage;
        _overflowPage = nullptr;
      }

      _bDeleted = true;
    }

    _bysVal = nullptr;
    delete _recLock;
    _recLock = nullptr;

    while (lr != nullptr) {
      if (lr->_recLock == nullptr) {
      }
      if (brb && lr->_recLock->_status != RecordStatus::ROLLBACK) {
        break;
      }

      if (lr->_overflowPage != nullptr) {
        pPage->GetIndexTree()->RecyclePageId(lr->_overflowPage->GetPageId(),
                                             lr->_overflowPage->GetPageNum());
        delete lr->_overflowPage;
        lr->_overflowPage = nullptr;
      }

      LeafRecord *lr2 = lr->_recLock->_undoRec;
      delete lr;
      lr = lr2;
    }

    lrDel = lr;
  }

  delete _recLock;
  _recLock = lrDel->_recLock;
  _bysVal = lrDel->_bysVal;
  _bSole = lrDel->_bSole;

  lrDel->_recLock = nullptr;
  lrDel->_bysVal = nullptr;
  lrDel->_bSole = false;
}

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr) {
  os << "TotalLen=" << lr.GetTotalLength() << "  Keys=";

  os << std::uppercase << std::hex << std::setfill('0') << "0x";
  Byte *bys = lr._bysVal + UI16_2_LEN;
  for (uint32_t i = 0; i < lr.GetKeyLength(); i++) {
    os << std::setw(2) << bys++;
    if (i % 4 == 0)
      os << ' ';
  }

  return os;
}

} // namespace storage
