﻿#include "LeafRecord.h"
#include "../dataType/DataValueFactory.h"
#include "../pool/StoragePool.h"
#include "../statement/Statement.h"
#include "../utils/ErrorID.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include <boost/crc.hpp>

namespace storage {
static thread_local boost::crc_32_type crc32;

void SetValueStruct(RecStruct &recStru, ValueStruct *arValStr,
                    uint32_t fieldNum, uint32_t valVarLen, Byte ver) {
#ifdef SINGLE_VERSION
  uint32_t byNum = (fieldNum + 7) >> 3;
  Byte *bys = recStru._bysValStart;
  arValStr[0].bysNull = bys;
  arValStr[0].varFiledsLen = (uint32_t *)(bys + byNum);
  arValStr[0].bysValue = bys + byNum + valVarLen;
#else
  Byte verNum = (*recStru._byVerFlow) & VERSION_NUM;
  uint32_t byNum = (fieldNum + 7) >> 3;
  Byte *bys = recStru._bysValStart;
  uint32_t offset = 0;
  for (Byte i = 0; i < verNum; i++) {
    if (ver == 0xff) {
      arValStr[i].bysNull = bys + offset;
      arValStr[i].varFiledsLen = (uint32_t *)(bys + offset + byNum);
      arValStr[i].bysValue = bys + offset + byNum + valVarLen;
    } else if (i == ver) {
      arValStr[0].bysNull = bys + offset;
      arValStr[0].varFiledsLen = (uint32_t *)(bys + offset + byNum);
      arValStr[0].bysValue = bys + offset + byNum + valVarLen;
      break;
    }
    offset += recStru._arrValLen[i];
  }
#endif
}
#ifdef SINGLE_VERSION
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
    if (ofPage != nullptr) {
      _bysValStart = ofPage->GetBysPage();
    }
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart = ((Byte *)_arrValLen) + UI32_LEN;
  }
}
#else
RecStruct::RecStruct(Byte *bys, uint16_t keyLen, Byte verNum,
                     OverflowPage *ofPage) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  _bysKey = bys + UI16_2_LEN;
  _byVerFlow = bys + UI16_2_LEN + keyLen;
  _arrStamp = (uint64_t *)(_byVerFlow + 1);
  _arrValLen = (uint32_t *)(((Byte *)_arrStamp) + UI64_LEN * verNum);

  if ((*_byVerFlow) & REC_OVERFLOW) [[unlikely]] {
    _arrCrc32 = (uint32_t *)(((Byte *)_arrValLen) + UI32_LEN * verNum);
    _pidStart = (PageID *)(((Byte *)_arrCrc32) + UI32_LEN);
    _pageNum = (uint16_t *)(((Byte *)_pidStart) + UI32_LEN);
    if (ofPage != nullptr) {
      _bysValStart = ofPage->GetBysPage();
    }
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart = ((Byte *)_arrValLen) + UI32_LEN * verNum;
  }
}
#endif

RecStruct::RecStruct(Byte *bys, OverflowPage *ofPage) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  uint16_t keyLen = *_keyLen;
  _bysKey = bys + UI16_2_LEN;
  _byVerFlow = bys + UI16_2_LEN + keyLen;
#ifdef SINGLE_VERSION
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
#else
  Byte verNum = (*_byVerFlow) & VERSION_NUM;
  _arrStamp = (uint64_t *)(_byVerFlow + 1);
  _arrValLen = (uint32_t *)(((Byte *)_arrStamp) + UI64_LEN * verNum);

  if ((*_byVerFlow) & REC_OVERFLOW) [[unlikely]] {
    assert(overPage != nullptr);
    _arrCrc32 = (uint32_t *)(((Byte *)_arrValLen) + UI32_LEN * verNum);
    _pidStart = (PageID *)(((Byte *)_arrCrc32) + UI32_LEN * verNum);
    _pageNum = (uint16_t *)(((Byte *)_pidStart) + UI32_LEN);
    assert(ofPage != nullptr);
    _bysValStart = overPage->GetBysPage();
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart = ((Byte *)_arrValLen) + UI32_LEN * verNum;
  }
#endif
}

LeafRecord::LeafRecord(IndexType idxType, Byte *bys)
    : RawRecord(bys, false, idxType) {}

LeafRecord::LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey,
                       Byte *bysPri, uint32_t lenPri, ActionType actType,
                       Statement *stmt, uint64_t recStamp)
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->ReadIndexType()) {
  _recLock =
      new RecordLock(ActionType::INSERT, RecordStatus::INIT, false, stmt);

  int i;
  uint16_t lenKey = CalcKeyLength(vctKey);

  int totalLen = lenKey + lenPri + UI16_2_LEN + UI64_LEN;
  _bysVal = CachePool::Apply(totalLen);
  Byte *bys = _bysVal;
  *((uint16_t *)bys) = totalLen;
  bys += UI16_LEN;
  *((uint16_t *)bys) = lenKey;

  for (i = 0; i < vctKey.size(); i++) {
    uint16_t len = vctKey[i]->WriteData(bys, SavePosition::KEY);
    bys += len;
  }

  BytesCopy(bys, bysPri, lenPri);
  bys += lenPri;
  *((uint64_t *)bys) = recStamp;
}

LeafRecord::LeafRecord(IndexTree *idxTree, const VectorDataValue &vctKey,
                       const VectorDataValue &vctVal, uint64_t recStamp,
                       Statement *stmt)
    : RawRecord(nullptr, true, idxTree->GetHeadPage()->ReadIndexType()) {
  _recLock =
      new RecordLock(ActionType::INSERT, RecordStatus::INIT, false, stmt);

  uint32_t lenKey = CalcKeyLength(vctKey);
  uint32_t lenVal = CalcValueLength(vctVal, ActionType::INSERT);
  uint16_t infoLen = 1 + UI64_LEN + UI32_LEN;
  uint32_t max_lenVal =
      (uint32_t)Configure::GetMaxRecordLength() - lenKey - UI16_2_LEN - infoLen;

  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::CACHE_PAGE_SIZE - 1) / CachePage::CACHE_PAGE_SIZE;
    _overflowPage =
        OverflowPage::GetPage(idxTree, idxTree->ApplyPageId(num), num, true);
    infoLen += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen =
      UI16_2_LEN + lenKey + infoLen +
      (_overflowPage == nullptr ? lenVal : UI32_LEN * 2 + UI16_LEN);

  _bysVal = CachePool::Apply(totalLen);
#ifdef SINGLE_VERSION
  RecStruct recStru(_bysVal, lenKey, _overflowPage);
#else
  RecStruct recStru(_bysVal, lenKey, 1, _overflowPage);
#endif
  FillHeaderBuff(recStru, totalLen, lenKey, 1, recStamp, lenVal,
                 ActionType::INSERT);
  FillKeyBuff(recStru, vctKey);

  ValueStruct valStru;
  SetValueStruct(recStru, &valStru, (uint32_t)vctVal.size(),
                 idxTree->GetValVarLen(), 0);

  FillValueBuff(valStru, vctVal);
  if (_overflowPage != nullptr) {
    crc32.reset();
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();
    _overflowPage->SetPageStatus(PageStatus::WRITING);
    // TO DO (Add to FilePagePool);
  }
}

LeafRecord::LeafRecord(LeafRecord &&src)
    : RawRecord(std::move(src)), _recLock(src._recLock),
      _overflowPage(src._overflowPage) {
  src._recLock = nullptr;
  src._overflowPage = nullptr;
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
int32_t LeafRecord::UpdateRecord(IndexTree *idxTree,
                                 const VectorDataValue &vctVal,
                                 uint64_t recStamp, Statement *stmt,
                                 ActionType type, bool gapLock) {
  assert(type == ActionType::UPDATE || type == ActionType::DELETE);
  RecStruct recStruOld(_bysVal, _overflowPage);
  RecordLock *recLock =
      new RecordLock(type, RecordStatus::SEATED, gapLock, stmt);
  recLock->_undoRec = new LeafRecord(move(*this));
  _recLock = recLock;
#ifdef SINGLE_VERSION
  uint32_t lenVal = CalcValueLength(vctVal, type);
  uint32_t lenInfo = 1 + (UI64_LEN + UI32_LEN);
  uint32_t max_lenVal = (uint32_t)Configure::GetMaxRecordLength() -
                        *recStruOld._keyLen - UI16_2_LEN - lenInfo;
  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::CACHE_PAGE_SIZE - 1) / CachePage::CACHE_PAGE_SIZE;
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
    SetValueStruct(recStru, &valStru, (uint32_t)vctVal.size(),
                   idxTree->GetValVarLen(), 0);
    FillValueBuff(valStru, vctVal);
  }

  if (_overflowPage != nullptr) {
    crc32.reset();
    crc32.process_bytes(recStru._bysValStart, lenVal);
    recStru._arrCrc32[0] = crc32.checksum();
    // TO DO (Add overflow page into FilePagePool)
    StoragePool::AddPage(_overflowPage, true);
  }
#else
  MVector<Byte> vctSn;
  uint32_t oldLenVal = CalcValidValueLength(recStruOld, true, vctSn);

  if (oldLenVal == 0 && type == ActionType::DELETE) {
    _bRemoved = true;
    return -(*recStruOld._totalLen);
  }

  uint32_t lenVal = CalcValueLength(vctVal, type);
  uint32_t lenInfo = 1 + (UI64_LEN + UI32_LEN) * (1 + (uint32_t)vctSn.size());
  uint32_t max_lenVal = (uint32_t)Configure::GetMaxRecordLength() -
                        *recStruOld._keyLen - UI16_2_LEN - lenInfo;
  if (lenVal + oldLenVal > max_lenVal) {
    uint16_t num = (lenVal + oldLenVal + CachePage::CACHE_PAGE_SIZE - 1) /
                   CachePage::CACHE_PAGE_SIZE;
    _overflowPage = OverflowPage::GetPage(
        _indexTree, _indexTree->ApplyPageId(num), num, true);
    lenInfo += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen = UI16_2_LEN + *recStruOld._keyLen + lenInfo +
                      (_overflowPage == nullptr ? lenVal + oldLenVal : 0);
  _bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(_bysVal, *recStruOld._keyLen, 1 + (uint32_t)vctSn.size(),
                    _overflowPage);
  FillHeaderBuff(recStru, totalLen, *recStruOld._keyLen,
                 1 + (uint32_t)vctSn.size(), recStamp, lenVal, type);
  BytesCopy(recStru._bysKey, recStruOld._bysKey, *recStruOld._keyLen);

  if (type == ActionType::UPDATE) {
    ValueStruct valStru;
    SetValueStruct(recStru, &valStru, (uint32_t)vctVal.size(),
                   _indexTree->GetValVarLen(), 0);
    FillValueBuff(valStru, vctVal);
  }
  if (vctSn.size() > 0) {
    uint32_t count = lenVal;
    ValueStruct arrStru[8];
    SetValueStruct(recStruOld, arrStru, (uint32_t)vctVal.size(),
                   _indexTree->GetValVarLen());

    for (size_t i = 0; i < vctSn.size(); i++) {
      Byte sn = vctSn[i];
      recStru._arrStamp[i + 1] = recStruOld._arrStamp[sn];
      recStru._arrValLen[i + 1] = recStruOld._arrValLen[sn];
      BytesCopy(recStru._bysValStart + count, arrStru[sn].bysNull,
                recStruOld._arrValLen[sn]);
      count += recStruOld._arrValLen[sn];
    }
  }

  if (_overflowPage != nullptr) {
    crc32.reset();
    crc32.process_bytes(recStru._bysValStart, lenVal + oldLenVal);
    recStru._arrCrc32[0] = crc32.checksum();
    // TO DO (Add overflow page into FilePagePool)
    StoragePool::AddPage(_overflowPage, true);
  }
#endif

  return *recStru._totalLen - *recStruOld._totalLen;
}

/**
 * @brief To calc the value length for move to new record. Here will remove
 * the repeated version
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
 * @brief Read the value to data value list, only used for parmary index
 * @param vctPos the fields positions in record that need to read
 * @param vctVal The vector to save the data values.
 * @param verStamp The version stamp for primary index.
 * @param tran To judge if it is the same transaction
 * @param bQuery If it is only query. If not only query, it will return record
 * with same transaction or return fail. If only query, it will return old
 * record with different transaction and new record with same transaction.
 * @return -1: Failed to read values due to no right to visit it or be locked;
 *         -2: No version to fit and failed to read
 *         0: Passed to read values with all fields.
 *         1: The record has been deleted
 */
ResultReadValue LeafRecord::ReadListValue(const MVector<int> &vctPos,
                                          VectorDataValue &vctVal,
                                          uint64_t verStamp, Statement *stmt,
                                          ActionType atype) const {
  assert(_indexType == IndexType::PRIMARY);

  const LeafRecord *lr = nullptr;
  if (_statement == nullptr || _statement == stmt ||
      _statement->GetTransactionStatus() == TranStatus::CLOSED ||
      (bQuery && _actionType == ActionType::QUERY_UPDATE) ||
      (_bRemoved && _undoRec == nullptr)) {
    lr = this;
  } else if (_statement->GetTransactionStatus() == TranStatus::FAILED ||
             _statement->GetTransactionStatus() == TranStatus::ROLLBACK ||
             (bQuery && (_actionType == ActionType::UPDATE ||
                         _actionType == ActionType::DELETE))) {
    lr = this;
    while (lr->_undoRec != nullptr) {
      lr = lr->_undoRec;
    }
    if (lr->_statement != nullptr &&
        lr->_actionType != ActionType::QUERY_UPDATE) {
      return -1;
    }
  } else {
    return -1;
  }

  RecStruct recStru(lr->_bysVal, lr->_overflowPage);
  Byte ver = 0;
  Byte verNum = *(recStru._byVerFlow) & 0x0f;
  for (; ver < verNum; ver++) {
    if (recStru._arrStamp[ver] <= verStamp) {
      break;
    }
  }

  if (ver == verNum) {
    return -2;
  }

  VersionStamp verSt = 0;
  set<VersionStamp, KeyCmp> setVer =
      _indexTree->GetHeadPage()->GetSetVerStamp();
  auto iter = setVer.lower_bound(recStru._arrStamp[ver]);
  if (iter != setVer.end()) {
    verSt = *iter;
  }
  for (; ver < verNum; ver++) {
    if (verSt > recStru._arrStamp[ver])
      break;
  }

  ver--;
  vctVal.clear();
  if (recStru._arrValLen[ver] == 0) {
    return 1;
  }

  ValueStruct valStru;
  const VectorDataValue &vdSrc = _indexTree->GetVctValue();
  SetValueStruct(recStru, &valStru, (uint32_t)vdSrc.size(),
                 _indexTree->GetValVarLen(), ver);
  assert((*recStru._byVerFlow & REC_OVERFLOW) == 0 || _overflowPage != nullptr);

  int varField = -1;
  Byte *bys = valStru.bysValue;
  int ipos = 0;
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
    if (vctPos.size() == 0 || vctPos[ipos] == i) {
      IDataValue *dv = vdSrc[i]->Clone();
      if (flen > 0)
        dv->ReadData(bys, flen, SavePosition::VALUE);

      vctVal.push_back(dv);
      ipos++;
    }

    bys += flen;
  }

  return 0;
}

RawKey *LeafRecord::GetKey() const {
  return new RawKey(_bysVal + _indexTree->GetKeyOffset(),
                    GetKeyLength() - _indexTree->GetKeyVarLen());
}

RawKey *LeafRecord::GetPrimayKey() const {
  int start = GetKeyLength() + UI16_2_LEN;
  int len = GetTotalLength() - start;
  Byte *buf = CachePool::Apply(len);
  BytesCopy(buf, _bysVal + start, len);
  return new RawKey(buf, len, true);
}

int LeafRecord::CompareTo(const LeafRecord &lr) const {
  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                      GetTotalLength() - _indexTree->GetKeyOffset(),
                      lr._bysVal + _indexTree->GetKeyOffset(),
                      lr.GetTotalLength() - _indexTree->GetKeyOffset());
}

int LeafRecord::CompareKey(const RawKey &key) const {
  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                      GetKeyLength() - _indexTree->GetKeyVarLen(),
                      key.GetBysVal(), key.GetLength());
}

int LeafRecord::CompareKey(const LeafRecord &lr) const {
  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                      GetKeyLength() - _indexTree->GetKeyVarLen(),
                      lr.GetBysValue() + _indexTree->GetKeyOffset(),
                      lr.GetKeyLength() - _indexTree->GetKeyVarLen());
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

uint32_t LeafRecord::CalcValueLength(const VectorDataValue &vctVal,
                                     ActionType type) {
  assert(_indexType == IndexType::PRIMARY);
  if (type == ActionType::DELETE)
    return 0;

  uint32_t lenVal =
      (uint32_t)(vctVal.size() + 7) / 8 + _indexTree->GetValVarLen();
  for (size_t i = 0; i < vctVal.size(); i++) {
    lenVal += vctVal[i]->GetPersistenceLength(SavePosition::VALUE);
  }
  return lenVal;
}

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
  _overflowPage = OverflowPage::GetPage(_indexTree, pid, pnum, false);
  return true;
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

  VectorDataValue vctVal;
  lr.GetListValue(vctVal);
  os << "  Values=";
  for (IDataValue *dv : vctVal) {
    os << *dv << "; ";
  }

  return os;
}
} // namespace storage
