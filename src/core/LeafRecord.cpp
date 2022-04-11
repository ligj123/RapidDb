﻿#include "LeafRecord.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../transaction/Transaction.h"
#include "../utils/ErrorMsg.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
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

  int totalLen = lenKey + lenPri + TWO_SHORT_LEN;
  _bysVal = CachePool::Apply(totalLen);
  *((uint16_t *)_bysVal) = totalLen;
  *((uint16_t *)(_bysVal + SHORT_LEN)) = lenKey;

  uint16_t pos = _indexTree->GetKeyOffset();
  uint16_t lenPos = TWO_SHORT_LEN;

  for (i = 0; i < vctKey.size(); i++) {
    uint16_t len = vctKey[i]->WriteData(_bysVal + pos, true);
    if (!vctKey[i]->IsFixLength()) {
      *((uint16_t *)(_bysVal + lenPos)) = len;
      lenPos += sizeof(uint16_t);
    }

    pos += len;
  }

  pos = TWO_SHORT_LEN + lenKey;
  BytesCopy(_bysVal + pos, bysPri, lenPri);
}

LeafRecord::LeafRecord(IndexTree *indexTree, const VectorDataValue &vctKey,
                       const VectorDataValue &vctVal, uint64_t recStamp,
                       Transaction *tran, LeafRecord *undoRec)
    : RawRecord(indexTree, nullptr, nullptr, true), _undoRec(undoRec),
      _tran(tran), _actionType(ActionType::INSERT) {

  int i = 0;
  uint16_t lenKey = _indexTree->GetKeyVarLen();
  for (i = 0; i < vctKey.size(); i++) {
    lenKey += vctKey[i]->GetPersistenceLength(true);
  }

  if (lenKey > Configure::GetMaxKeyLength()) {
    throw ErrorMsg(CORE_EXCEED_KEY_LENGTH, {std::to_string(lenKey)});
  }

  uint16_t infoLen = 1 + 1 + 8 + 4;
  uint32_t lenVal = (vctVal.size() + 7) / 8 + _indexTree->GetValVarLen();
  uint32_t max_lenVal =
      (uint32_t)Configure::GetMaxRecordLength() - lenKey - TWO_SHORT_LEN;
  for (i = 0; i < vctVal.size(); i++) {
    lenVal += vctVal[i]->GetPersistenceLength(false);
  }

  if (infoLen + lenVal > max_lenVal) {
  }

  uint16_t lenAttr = 1 + sizeof(uint64_t) * rsCount;
  if (sizeOverflow > 0 || lenVal + snapLen > max_lenVal) {
    lenAttr += sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t) +
               sizeof(uint16_t) + sizeof(uint32_t) * rsCount;
  } else {
    lenAttr += sizeof(uint16_t) * rsCount;
  }

  _priStru = new PriValStruct;
  _priStru->bLastOvf = (sizeOverflow > 0);
  _priStru->bOvf = (sizeOverflow > 0 ||
                    (lenAttr + lenVal + snapLen > max_lenVal && snapLen > 0));
  _priStru->bDelete = (type == ActionType::DELETE);
  _priStru->verCount = rsCount;

  int totalLen = lenKey + sizeof(uint16_t) * 2 + lenAttr + lenVal;
  if (!_priStru->bOvf)
    totalLen += snapLen;

  _bysVal = CachePool::Apply(totalLen);
  if (type == ActionType::DELETE && rsCount == 1) {
    *((uint16_t *)_bysVal) = 0;
    return;
  }
  *((uint16_t *)_bysVal) = totalLen;
  *((uint16_t *)(_bysVal + sizeof(uint16_t))) = lenKey;

  uint16_t pos = _indexTree->GetKeyOffset();
  uint16_t lenPos = TWO_SHORT_LEN;

  for (i = 0; i < vctKey.size(); i++) {
    uint16_t len = vctKey[i]->WriteData(_bysVal + pos, true);
    if (!vctKey[i]->IsFixLength()) {
      *((uint16_t *)(_bysVal + lenPos)) = len;
      lenPos += sizeof(uint16_t);
    }

    pos += len;
  }

  _priStru->pageValOffset = pos + lenAttr;

  _bysVal[pos] = (_priStru->bLastOvf ? LAST_RECORD_OVERFLOW : 0) +
                 (_priStru->bOvf ? RECORD_OVERFLOW : 0) +
                 (_priStru->bDelete ? RECORD_DELETED : 0) + _priStru->verCount;
  pos++;

  _priStru->arrStamp = (uint64_t *)&_bysVal[pos];
  pos += sizeof(uint64_t) * rsCount;
  _priStru->arrStamp[0] = recStamp;

  if (rsCount > 1) {
    BytesCopy(&_priStru->arrStamp[1], &oldValStru->arrStamp[bOldAll ? 0 : 1],
              sizeof(uint64_t) * (rsCount - 1));
  }

  if (!_priStru->bOvf) {
    _priStru->arrPageLen = (uint16_t *)&_bysVal[pos];
    _priStru->arrPageLen[0] = lenVal;

    uint16_t oldStart = (bOldAll ? 0 : oldValStru->arrPageLen[0]);
    int oldIndex = (bOldAll ? 0 : 1);
    for (int i = 1; i < rsCount; i++) {
      _priStru->arrPageLen[i] = _priStru->arrPageLen[i - 1] +
                                oldValStru->arrPageLen[oldIndex] - oldStart;
      oldStart = oldValStru->arrPageLen[oldIndex];
      oldIndex++;
    }

    pos += sizeof(uint16_t) * rsCount;

    for (int i = 0; i < vctVal.size(); i++) {
      pos += vctVal[i]->WriteData(_bysVal + pos, false);
    }

    oldStart = (bOldAll ? 0 : oldValStru->arrPageLen[0]);
    oldIndex = (bOldAll ? 0 : 1);
    for (int i = 1; i < rsCount; i++) {
      BytesCopy(_bysVal + _priStru->pageValOffset + _priStru->arrPageLen[i - 1],
                old->_bysVal + oldValStru->pageValOffset + oldStart,
                _priStru->arrPageLen[i] - _priStru->arrPageLen[i - 1]);
      oldStart = oldValStru->arrPageLen[oldIndex];
      oldIndex++;
    }
  } else {
    _priStru->indexOvfStart = indexOvfStart;
    _priStru->lenInPage = lenVal;
    _priStru->arrOvfLen = (uint32_t *)&_bysVal[pos + sizeof(uint64_t) * 2];
    _priStru->arrOvfLen[0] = sizeOverflow;

    if (oldValStru != nullptr) {
      if (oldValStru->bOvf) {
        uint32_t oldStart = (bOldAll ? 0 : oldValStru->arrOvfLen[0]);
        int oldIndex = (bOldAll ? 0 : 1);
        for (int i = 1; i < rsCount; i++) {
          _priStru->arrOvfLen[i] = _priStru->arrOvfLen[i - 1] +
                                   oldValStru->arrOvfLen[oldIndex] - oldStart;
          oldStart = oldValStru->arrOvfLen[oldIndex];
          oldIndex++;
        }
      } else {
        uint32_t oldStart = (bOldAll ? 0 : oldValStru->arrPageLen[0]);
        int oldIndex = (bOldAll ? 0 : 1);
        for (int i = 1; i < rsCount; i++) {
          _priStru->arrOvfLen[i] = _priStru->arrOvfLen[i - 1] +
                                   oldValStru->arrPageLen[oldIndex] - oldStart;
          oldStart = oldValStru->arrPageLen[oldIndex];
          oldIndex++;
        }
      }
    }

    PageFile *ovf = indexTree->GetOverflowFile();

    if (oldValStru == nullptr || !oldValStru->bOvf ||
        oldValStru->ovfRange < sizeOverflow + snapLen) {
      _priStru->ovfRange =
          sizeOverflow + snapLen + (uint32_t)Configure::GetDiskClusterSize();
      _priStru->ovfRange -=
          _priStru->ovfRange % (uint32_t)Configure::GetDiskClusterSize();
      _priStru->ovfOffset = ovf->GetOffsetAddLength(_priStru->ovfRange);
    } else {
      _priStru->ovfRange = oldValStru->ovfRange;
      _priStru->ovfOffset = oldValStru->ovfOffset;
    }

    UInt64ToBytes(_priStru->ovfOffset, _bysVal + pos, false);
    pos += sizeof(uint64_t);
    UInt32ToBytes(_priStru->ovfRange, _bysVal + pos, false);
    pos += sizeof(uint32_t);
    UInt16ToBytes(indexOvfStart, _bysVal + pos, false);
    pos += sizeof(uint16_t);
    UInt16ToBytes(lenVal, _bysVal + pos, false);
    pos += sizeof(uint16_t);

    pos += sizeof(uint32_t) * rsCount;

    for (int i = 0; i < indexOvfStart; i++) {
      pos += vctVal[i]->WriteData(_bysVal + pos, false);
    }

    ovf->WriteDataValue(vctVal, indexOvfStart, _priStru->ovfOffset);

    if (rsCount > 1) {
      if (!oldValStru->bOvf) {
        uint32_t valStart = oldValStru->pageValOffset +
                            (bOldAll ? 0 : oldValStru->arrPageLen[0]);
        uint32_t valLen = oldValStru->pageValOffset +
                          oldValStru->arrPageLen[oldValStru->verCount - 1] -
                          valStart;
        ovf->WritePage(_priStru->ovfOffset + sizeOverflow,
                       (char *)(old->_bysVal + valStart), valLen);
      } else {
        uint64_t offset = _priStru->ovfOffset + sizeOverflow;
        if (bOldAll) {
          ovf->WritePage(offset,
                         (char *)(old->_bysVal + oldValStru->pageValOffset),
                         oldValStru->lenInPage);
          offset += oldValStru->lenInPage;
        }

        uint64_t valStart =
            oldValStru->ovfOffset + (bOldAll ? 0 : oldValStru->arrOvfLen[0]);
        uint64_t valLen =
            oldValStru->arrOvfLen[oldValStru->verCount - 1] - valStart;

        ovf->MoveOverflowData(valStart, offset, (uint32_t)valLen);
      }
    }
  }
}

LeafRecord::~LeafRecord() {
  LeafRecord *rec = _undoRecords;
  while (rec != nullptr) {
    LeafRecord *next = rec->_undoRecords;
    rec->ReleaseRecord();
    rec = next;
  }

  if (_priStru != nullptr)
    delete _priStru;
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
/**
 * @brief Read the value to data value list
 * @param vct The vector to save the data values.
 * @param verStamp The version stamp for primary index.If not primary, not to
 * use.
 * @param tran To judge if it is the same transaction
 * @param bQuery If it is only query. If not only query, it will return record
 * with same transaction or return fail. If only query, it will return old
 * record with different transaction and new record with same transaction.
 * @return -1: Failed to read values due to no right version stamp for
 * primary or locked; 0: Passed to read values with all fields. 1: Passed to
 read part of values due to same of fields saved in overflow file.
 */
int LeafRecord::GetListValue(VectorDataValue &vctVal, uint64_t verStamp,
                             Transaction *tran, bool bQuery) const {
  _indexTree->CloneValues(vctVal);
  bool bPri =
      (_indexTree->GetHeadPage()->ReadIndexType() == IndexType::PRIMARY);
  if (!bPri) {
    if (_tran != nullptr && _tran != tran)
      return -1;

    uint16_t lenPos = TWO_SHORT_LEN + GetKeyLength();
    uint16_t pos = lenPos + _indexTree->GetValVarLen();
    uint16_t len = 0;

    for (uint16_t i = 0; i < vctVal.size(); i++) {
      if (!vctVal[i]->IsFixLength() && !bPri) {
        len = *((uint16_t *)(_bysVal + lenPos));
        lenPos += sizeof(uint16_t);
      }

      pos += vctVal[i]->ReadData(_bysVal + pos, len);
    }

    return 0;
  }

  const LeafRecord *lr = nullptr;
  if (_tran == nullptr || _tran == tran ||
      _tran->GetTransactionStatus() == TranStatus::CLOSED ||
      (bQuery && _actionType == ActionType::QUERY_UPDATE)) {
    lr = this;
  } else if (tran->GetTransactionStatus() == TranStatus::FAILED ||
             tran->GetTransactionStatus() == TranStatus::ROLLBACK ||
             (bQuery && (_actionType == ActionType::INSERT ||
                         _actionType == ActionType::UPDATE ||
                         _actionType == ActionType::DELETE))) {
    lr = this;
    while (lr->_undoRecords != nullptr) {
      lr = lr->_undoRecords;
    }
    if (lr->_tran != nullptr && lr->_actionType != ActionType::QUERY_UPDATE) {
      return -1;
    }
  } else {
    return -1;
  }

  PriValStruct *priStru = lr->GetPriValStruct();
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

void LeafRecord::GetListOverflow(VectorDataValue &vctVal) const {
  uint16_t indexOvfStart = GetIndexOvfStart();
  if (indexOvfStart >= vctVal.size())
    return;

  PriValStruct *priStru = GetPriValStruct();
  PageFile *ovf = _indexTree->GetOverflowFile();
  ovf->ReadDataValue(vctVal, indexOvfStart, priStru->ovfOffset,
                     priStru->arrOvfLen[0]);
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

RawKey *LeafRecord::GetKey() const {
  return new RawKey(_bysVal + _indexTree->GetKeyOffset(),
                    GetKeyLength() - _indexTree->GetKeyVarLen());
}

RawKey *LeafRecord::GetPrimayKey() const {
  int start = GetKeyLength() + _indexTree->GetValOffset();
  int len = GetTotalLength() - start;
  Byte *buf = CachePool::Apply(len);
  BytesCopy(buf, _bysVal + start, len);
  return new RawKey(buf, len, true);
}

std::ostream &operator<<(std::ostream &os, const LeafRecord &lr) {
  VectorDataValue vctKey;
  lr.GetListKey(vctKey);
  os << "TotalLen=" << lr.GetTotalLength() << "  Keys=";
  for (IDataValue *dv : vctKey) {
    os << *dv << "; ";
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
