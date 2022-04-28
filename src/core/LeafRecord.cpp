#include "LeafRecord.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../transaction/Transaction.h"
#include "../utils/ErrorMsg.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include <boost/crc.hpp>

namespace storage {
static thread_local boost::crc_32_type crc32;

RecStruct::RecStruct(Byte *bys, uint16_t varKeyOff, uint16_t keyLen,
                     Byte verNum, OverflowPage *overPage) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  _varKeyLen = (uint16_t *)(bys + UI16_2_LEN);
  _bysKey = bys + UI16_2_LEN + varKeyOff;
  _byVerNum = bys + UI16_2_LEN + keyLen;
  _arrStamp = (uint64_t *)(bys + UI16_2_LEN + keyLen + 1);
  _arrValLen = (uint32_t *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum);

  if (overPage != nullptr) {
    _arrCrc32 = (uint32_t *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum +
                             UI32_LEN * verNum);
    _pidStart = (PageID *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum +
                           UI32_LEN * verNum * 2);
    _pageNum = (uint16_t *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum +
                            UI32_LEN * verNum * 2 + UI16_LEN);
    *_pidStart = overPage->GetPageId();
    *_pageNum = overPage->GetPageNum();
    _bysValStart = overPage->GetBysPage();
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart =
        bys + UI16_2_LEN + keyLen + 1 + (UI64_LEN + UI32_LEN) * verNum;
  }
}

RecStruct::RecStruct(Byte *bys, uint16_t varKeyOff,
                     OverflowPage *overPage = nullptr) {
  _totalLen = (uint16_t *)bys;
  _keyLen = (uint16_t *)(bys + UI16_LEN);
  uint16_t keyLen = *_keyLen;
  _varKeyLen = (uint16_t *)(bys + UI16_2_LEN);
  _bysKey = bys + UI16_2_LEN + varKeyOff;
  _byVerNum = bys + UI16_2_LEN + keyLen;
  Byte verNum = (*_byVerNum) & 0x0f;
  _arrStamp = (uint64_t *)(bys + UI16_2_LEN + keyLen + 1);
  _arrValLen = (uint32_t *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum);

  if ((*_byVerNum) & 0x80) {
    assert(overPage != nullptr);
    _arrCrc32 = (uint32_t *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum +
                             UI32_LEN * verNum);
    _pidStart = (PageID *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum +
                           UI32_LEN * verNum * 2);
    _pageNum = (uint16_t *)(bys + UI16_2_LEN + keyLen + 1 + UI64_LEN * verNum +
                            UI32_LEN * verNum * 2 + UI16_LEN);
    *_pidStart = overPage->GetPageId();
    *_pageNum = overPage->GetPageNum();
    _bysValStart = overPage->GetBysPage();
  } else {
    _arrCrc32 = nullptr;
    _pidStart = nullptr;
    _pageNum = nullptr;
    _bysValStart =
        bys + UI16_2_LEN + keyLen + 1 + (UI64_LEN + UI32_LEN) * verNum;
  }
}

LeafRecord::LeafRecord(LeafPage *parentPage, Byte *bys)
    : RawRecord(parentPage->GetIndexTree(), parentPage, bys, false) {}

LeafRecord::LeafRecord(IndexTree *indexTree, Byte *bys)
    : RawRecord(indexTree, nullptr, bys, false) {}

LeafRecord::LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
                       Byte *bysPri, uint32_t lenPri, ActionType type,
                       Transaction *tran)
    : RawRecord(indexTree, nullptr, nullptr, true), _tran(tran) {
  _actionType = type;

  int i;
  uint16_t lenKey = _indexTree->GetKeyVarLen();
  for (i = 0; i < vctKey.size(); i++) {
    lenKey += vctKey[i]->GetPersistenceLength(true);
  }

  if (lenKey > Configure::GetMaxKeyLength()) {
    throw ErrorMsg(CORE_EXCEED_KEY_LENGTH, {std::to_string(lenKey)});
  }

  int totalLen = lenKey + lenPri + UI16_2_LEN;
  _bysVal = CachePool::Apply(totalLen);
  *((uint16_t *)_bysVal) = totalLen;
  *((uint16_t *)(_bysVal + UI16_LEN)) = lenKey;

  uint16_t pos = _indexTree->GetKeyOffset();
  uint16_t lenPos = UI16_2_LEN;

  for (i = 0; i < vctKey.size(); i++) {
    uint16_t len = vctKey[i]->WriteData(_bysVal + pos, true);
    if (!vctKey[i]->IsFixLength()) {
      *((uint16_t *)(_bysVal + lenPos)) = len;
      lenPos += sizeof(uint16_t);
    }

    pos += len;
  }

  pos = UI16_2_LEN + lenKey;
  BytesCopy(_bysVal + pos, bysPri, lenPri);
}

LeafRecord::LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
                       const VectorDataValue &vctVal, uint64_t recStamp,
                       Transaction *tran)
    : RawRecord(indexTree, nullptr, nullptr, true), _tran(tran) {
  _actionType = ActionType::INSERT;

  uint32_t lenKey = CalcKeyLength(vctKey);
  uint32_t lenVal = CalcValueLength(vctVal, ActionType::INSERT);
  uint16_t infoLen = 1 + UI64_LEN + UI32_LEN;
  uint32_t max_lenVal =
      (uint32_t)Configure::GetMaxRecordLength() - lenKey - UI16_2_LEN - infoLen;

  if (lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::CACHE_PAGE_SIZE - 1) / CachePage::CACHE_PAGE_SIZE;
    _overflowPage =
        OverflowPage::GetPage(indexTree, indexTree->ApplyPageId(num), num);
    infoLen += UI32_LEN + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen =
      UI16_2_LEN + lenKey + infoLen + (_overflowPage == nullptr ? lenVal : 0);

  _bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(_bysVal, indexTree->GetKeyVarLen(), lenKey, 1,
                    _overflowPage);
  FillHeaderBuff(recStru, totalLen, lenKey, 1, recStamp, lenVal);
  FillKeyBuff(recStru, vctKey);

  ValueStruct valStru;
  SetValueStruct(recStru, &valStru, vctVal.size(), indexTree->GetValVarLen());

  FillValueBuff(valStru, vctVal);
  if (_overflowPage != nullptr) {
    crc32.reset();
    crc32.process_bytes(recStru._bysValStart, lenVal);
  }
}

LeafRecord::~LeafRecord() {
  if (_undoRec != nullptr) {
    _undoRec->ReleaseRecord();
  }
}

void LeafRecord::ReleaseRecord(bool bUndo = false) {
  _refCount--;
  if (_refCount == 0) {
    if (_overflowPage != nullptr) {
      if (bUndo) {
        _indexTree->ReleasePageId(_overflowPage->GetPageId(),
                                  _overflowPage->GetPageNum());
      }
      _overflowPage->DecRefCount();
    }
    delete this;
  }
}

LeafRecord::LeafRecord(LeafRecord &src)
    : RawRecord(src), _undoRec(src._undoRec), _tran(src._tran),
      _overflowPage(src._overflowPage) {
  src._undoRec = nullptr;
  src._tran = nullptr;
  _overflowPage = nullptr;
}

void LeafRecord::GetListKey(VectorDataValue &vctKey) const {
  _indexTree->CloneKeys(vctKey);
  uint16_t pos = _indexTree->GetKeyOffset();
  uint16_t lenPos = TWO_SHORT_LEN;
  uint16_t len = 0;

  for (uint16_t i = 0; i < vctKey.size(); i++) {
    if (!vctKey[i]->IsFixLength()) {
      len = *((uint16_t *)(_bysVal + lenPos));
      lenPos += sizeof(uint16_t);
    }

    pos += vctKey[i]->ReadData(_bysVal + pos, len);
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
int32_t LeafRecord::UpdateRecord(const VectorDataValue &vctVal,
                                 uint64_t recStamp, Transaction *tran,
                                 ActionType type) {
  assert(_refCount.load(memory_order_relaxed) == 1);
  RecStruct recStruOld(_bysVal, _indexTree->GetKeyVarLen(), _overflowPage);
  MVector<Byte>::Type vctSn;
  uint32_t oldLenVal = CalcValidValueLength(recStruOld, true, vctSn);
  _actionType = type;

  if (oldLenVal == 0 && type == ActionType::DELETE) {
    return -(*recStruOld._totalLen);
  }

  _undoRec = new LeafRecord(*this);
  uint32_t lenVal = CalcValueLength(vctVal, type);
  uint32_t lenInfo = 1 + (UI64_LEN + UI32_LEN) * (1 + vctSn.size());
  uint32_t max_lenVal = (uint32_t)Configure::GetMaxRecordLength() -
                        *recStruOld._keyLen - TWO_SHORT_LEN - lenInfo;
  if (lenVal + oldLenVal > max_lenVal) {
    uint16_t num = (lenVal + oldLenVal + CachePage::CACHE_PAGE_SIZE - 1) /
                   CachePage::CACHE_PAGE_SIZE;
    _overflowPage =
        OverflowPage::GetPage(_indexTree, _indexTree->ApplyPageId(num), num);
    lenInfo += UI32_LEN * (1 + vctSn.size()) + UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen = UI16_2_LEN + *recStruOld._keyLen + lenInfo +
                      (_overflowPage == nullptr ? lenVal + oldLenVal : 0);

  _bysVal = CachePool::Apply(totalLen);
  RecStruct recStru(_bysVal, _indexTree->GetKeyVarLen(), *recStruOld._keyLen,
                    1 + vctSn.size(), _overflowPage);
  FillHeaderBuff(recStru, totalLen, *recStruOld._keyLen, 1 + vctSn.size(),
                 recStamp, lenVal);
  BytesCopy(recStru._bysKey, recStruOld._bysKey, *recStruOld._keyLen);

  if (type == ActionType::UPDATE) {
    ValueStruct valStru;
    SetValueStruct(recStru, &valStru, vctVal.size(),
                   _indexTree->GetValVarLen());
    FillValueBuff(valStru, vctVal);
    if (_overflowPage != nullptr) {
      crc32.reset();
      crc32.process_bytes(valStru.bysNull, lenVal);
      recStru._arrCrc32[0] = crc32.checksum();
    }
  } else if (_overflowPage != nullptr) {
    recStru._arrCrc32[0] = 0;
  }

  if (vctSn.size() > 0) {
    uint32_t count = lenVal;
    ValueStruct arrStru[8];
    SetValueStruct(recStru, arrStru, vctVal.size(), _indexTree->GetValVarLen());

    for (size_t i = 1; i < vctSn.size(); i++) {
      Byte sn = vctSn[i];
      recStru._arrStamp[i] = recStruOld._arrStamp[sn];
      recStru._arrValLen[i] = recStruOld._arrValLen[sn];
      BytesCopy(recStru._bysValStart + count, arrStru[sn].bysNull,
                recStruOld._arrValLen[sn]);
      count += recStruOld._arrValLen[sn];
    }
  }

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
uint32_t LeafRecord::CalcValidValueLength(RecStruct &recStru, bool bUpdate,
                                          MVector<Byte>::Type &vctSN) {
  const set<VersionStamp> &setVer = _indexTree->GetHeadPage()->GetSetVerStamp();
  if (setVer.size() == 0)
    return 0;

  uint32_t len = 0;

  Byte verNum = (*recStru._byVerNum) & 0x0f;

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
 * -2: No version to fit and failed to read
 * 0:Passed to read values with all fields.
 */
int LeafRecord::GetListValue(const MVector<int> &vctPos,
                             VectorDataValue &vctVal, uint64_t verStamp,
                             Transaction *tran, bool bQuery) const {
  const VectorDataValue &vctDv = _indexTree->GetVctValue();
  assert(_indexTree->GetHeadPage()->ReadIndexType() == IndexType::PRIMARY);

  const LeafRecord *lr = nullptr;
  if (_tran == nullptr || _tran == tran ||
      _tran->GetTransactionStatus() == TranStatus::CLOSED ||
      (bQuery && _actionType == ActionType::QUERY_UPDATE) ||
      (_bRemoved && _undoRec == nullptr)) {
    lr = this;
  } else if (tran->GetTransactionStatus() == TranStatus::FAILED ||
             tran->GetTransactionStatus() == TranStatus::ROLLBACK ||
             (bQuery && (_actionType == ActionType::UPDATE ||
                         _actionType == ActionType::DELETE))) {
    lr = this;
    while (lr->_undoRec != nullptr) {
      lr = lr->_undoRec;
    }
    if (lr->_tran != nullptr && lr->_actionType != ActionType::QUERY_UPDATE) {
      return -1;
    }
  } else {
    return -1;
  }

  RecStruct recStru(lr->_bysVal, _indexTree->GetKeyVarLen(), lr->_overflowPage);
  Byte ver = 0;
  for (; ver < priStru->verCount; ver++) {
    if (priStru->arrStamp[ver] <= verStamp) {
      break;
    }
  }

  if (ver >= priStru->verCount) {
    return -1;
  }

  int hr = 0;
  if (ver == 0 || !priStru->bOvf) {
    int indexOvfStart = (int)vctVal.size();
    int pos = 0;
    int len = 0;
    if (ver == 0) {
      indexOvfStart = lr->GetIndexOvfStart();
      if (indexOvfStart >= vctVal.size()) {
        indexOvfStart = (int)vctVal.size();
      } else {
        hr = 1;
      }
      pos = priStru->pageValOffset;
      len = (priStru->bOvf ? priStru->lenInPage : priStru->arrPageLen[0]);
    } else {
      pos = priStru->pageValOffset + priStru->arrPageLen[ver - 1];
      len = priStru->arrPageLen[ver];
    }

    if (len == 0) {
      vctVal.RemoveAll();
    } else {
      for (uint16_t i = 0; i < indexOvfStart; i++) {
        pos += vctVal[i]->ReadData(lr->_bysVal + pos, -1);
      }
    }
  } else {
    uint64_t offset = priStru->ovfOffset + priStru->arrOvfLen[ver - 1];
    uint32_t totalLen = priStru->arrOvfLen[ver] - priStru->arrOvfLen[ver - 1];

    PageFile *ovf = _indexTree->GetOverflowFile();
    ovf->ReadDataValue(vctVal, 0, offset, totalLen);
  }

  return hr;
}
//
// void LeafRecord::GetListOverflow(VectorDataValue &vctVal) const {
//  uint16_t indexOvfStart = GetIndexOvfStart();
//  if (indexOvfStart >= vctVal.size())
//    return;
//
//  PriValStruct *priStru = GetPriValStruct();
//  PageFile *ovf = _indexTree->GetOverflowFile();
//  ovf->ReadDataValue(vctVal, indexOvfStart, priStru->ovfOffset,
//                     priStru->arrOvfLen[0]);
//}
//
// int LeafRecord::CompareTo(const LeafRecord &lr) const {
//  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
//                      GetTotalLength() - _indexTree->GetKeyOffset(),
//                      lr._bysVal + _indexTree->GetKeyOffset(),
//                      lr.GetTotalLength() - _indexTree->GetKeyOffset());
//}
//
// int LeafRecord::CompareKey(const RawKey &key) const {
//  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
//                      GetKeyLength() - _indexTree->GetKeyVarLen(),
//                      key.GetBysVal(), key.GetLength());
//}
//
// int LeafRecord::CompareKey(const LeafRecord &lr) const {
//  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
//                      GetKeyLength() - _indexTree->GetKeyVarLen(),
//                      lr.GetBysValue() + _indexTree->GetKeyOffset(),
//                      lr.GetKeyLength() - _indexTree->GetKeyVarLen());
//}
//
// RawKey *LeafRecord::GetKey() const {
//  return new RawKey(_bysVal + _indexTree->GetKeyOffset(),
//                    GetKeyLength() - _indexTree->GetKeyVarLen());
//}

// RawKey *LeafRecord::GetPrimayKey() const {
//  int start = GetKeyLength() + _indexTree->GetValOffset();
//  int len = GetTotalLength() - start;
//  Byte *buf = CachePool::Apply(len);
//  BytesCopy(buf, _bysVal + start, len);
//  return new RawKey(buf, len, true);
//}
//
// std::ostream &operator<<(std::ostream &os, const LeafRecord &lr) {
//  VectorDataValue vctKey;
//  lr.GetListKey(vctKey);
//  os << "TotalLen=" << lr.GetTotalLength() << "  Keys=";
//  for (IDataValue *dv : vctKey) {
//    os << *dv << "; ";
//  }
//
//  VectorDataValue vctVal;
//  lr.GetListValue(vctVal);
//  os << "  Values=";
//  for (IDataValue *dv : vctVal) {
//    os << *dv << "; ";
//  }
//
//  return os;
//}
} // namespace storage
