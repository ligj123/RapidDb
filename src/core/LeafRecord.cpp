#include "LeafRecord.h"
#include "../config/ErrorID.h"
#include "../dataType/DataValueFactory.h"
#include "../transaction/Transaction.h"
#include "../utils/ErrorMsg.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
struct ValueStruct {
  // (n+7)/8 bytes to save if the related fields are null or not, every field
  // occupys a bit.
  Byte *bysNull;
  // The variable fields' lengths after serialize
  uint32_t *varFiledsLen;
  // Start position to save value content
  Byte *bysValue;
};
// Only for primary index
struct RecStruct {
  RecStruct(Byte *bys, uint16_t varKeyOff, uint16_t keyLen, Byte verNum,
            bool bOverflow = false) {
    totalLen = (uint16_t *)bys;
    keyLen = (uint16_t *)(bys + UI16_LEN);
    varKeyLen = (uint16_t *)(bys + TWO_UI16_LEN);
    bysKey = bys + TWO_UI16_LEN + varKeyOff;
    byVerNum = bys + TWO_UI16_LEN + keyLen;
    arrStamp = (uint64_t *)(bys + TWO_UI16_LEN + keyLen + 1);
    arrLen = (uint32_t *)(bys + TWO_UI16_LEN + keyLen + 1 + UI64_LEN * verNum);

    if (bOverflow) {
      arrCrc32 = (uint32_t *)(bys + TWO_UI16_LEN + keyLen + 1 +
                              UI64_LEN * verNum + UI32_LEN * verNum);
      pidStart = (PageID *)(bys + TWO_UI16_LEN + keyLen + 1 +
                            UI64_LEN * verNum + UI32_LEN * verNum * 2);
      pageNum =
          (uint16_t *)(bys + TWO_UI16_LEN + keyLen + 1 + UI64_LEN * verNum +
                       UI32_LEN * verNum * 2 + UI16_LEN);
    } else {
      arrCrc32 = nullptr;
      pidStart = nullptr;
      pageNum = nullptr;
    }
  }

  void SetValueStruct(Byte *bys, Byte verNum, uin32_t *arLen, uint32_t fieldNum,
                      uint32_t valVarLen) {
    uint32_t byNum = (fieldNum + 7) << 3;
    uint32_t offset = 0;
    for (Byte i = 0; i < verNum; i++) {
      arvalStr[i].bysNull = bys + offset;
      arvalStr[i].varFiledsLen = bys + offset + byNum;
      arvalStr[i].bysValue = bys + offset + byNum + valVarLen;
    }
  }

  // To save total length for record, not include values in overflow page
  // length, only to calc the occupied bytes in LeafPage
  uint16_t *totalLen;
  // To save key length, include the bytes to save variable fileds length and
  // key content.
  uint16_t *keyLen;
  // The start position to save variable fileds length in key, this is an array
  // with 0~N elements.
  uint16_t *varKeyLen;
  // To save key content after serialize.
  Byte *bysKey;
  // The byte to save the number of versions(low 4bits) and if overflow or not
  // (the highest bit)
  Byte *byVerNum;
  // The array to save version stamps for every version, 1~N elements.
  uint64_t *arrStamp;
  // The array to save length for every version content.
  uint32_t *arrLen;
  // The array to save the crc32 if overflow
  uint32_t *arrCrc32;
  // The start page id
  PageID *pidStart;
  // The page number
  uint16_t *pageNum;
  // The array to save the address for value struct with max 8 elements.
  ValueStruct arvalStr[8];
};

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
                       Transaction *tran)
    : RawRecord(indexTree, nullptr, nullptr, true), _tran(tran) {
  _actionType = ActionType::INSERT;
  RecStruct recStru;

  uint32_t lenKey = CalcKeyLength(vctKey);
  uint32_t lenVal = CalcValueLength(vctVal);
  uint16_t infoLen = 1 + 1 + 8 + 4;
  uint32_t max_lenVal = (uint32_t)Configure::GetMaxRecordLength() - lenKey -
                        TWO_SHORT_LEN - infoLen;

  if (infoLen + lenVal > max_lenVal) {
    uint16_t num =
        (lenVal + CachePage::CACHE_PAGE_SIZE - 1) / CachePage::CACHE_PAGE_SIZE;
    _overflowPage =
        new OverflowPage(indexTree, indexTree->ApplyPageId(num), num);
    infoLen += UI32_LEN + UI16_LEN;
  }

  uint16_t totalLen =
      lenKey + infoLen + (_overflowPage == nullptr ? lenVal : 0);

  _bysVal = CachePool::Apply(totalLen);

  Byte *bys = FillKeyBuff(_bysVal, totalLen, lenKey, vctKey);

  *bys = (_overflowPage == nullptr ? 0 : 0xc0) + 1;
  bys++;
  *bys = 1 * (uint64_t *)bys2 = recStamp;
  bys += UI64_LEN;
  *(uint32_t *)bys = lenVal;
  bys += UI32_LEN;

  Byte *crc = nullptr;
  Byte *fBit = nullptr;
  if (_overflowPage != nullptr) {
    crc = bys2;
    bys2 += UI32_LEN;
    *(uint32_t *)bys2 = _overflowPage->GetPageId();
    bys2 += UI32_LEN;
    *(uint16_t *)bys2 = _overflowPage->GetPageNum();

    fBit = _overflowPage->GetBysPage();
  } else {
    fBit = bys2;
  }
  FillValueBuff(fBit, vctVal);
}

LeafRecord::~LeafRecord() {
  if (_undoRec != nullptr) {
    _undoRec->ReleaseRecord();
  }

  if (_overflowPage != nullptr) {
    _overflowPage->DecRefCount();
  }
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

void LeafRecord::UpdateRecord(const VectorDataValue &vctVal, uint64_t recStamp,
                              ActionType type, Transaction *tran) {
  uint32_t lenVal = CalcValueLength(vctVal);
}

///**
// * @brief Read the value to data value list
// * @param vct The vector to save the data values.
// * @param verStamp The version stamp for primary index.If not primary, not to
// * use.
// * @param tran To judge if it is the same transaction
// * @param bQuery If it is only query. If not only query, it will return record
// * with same transaction or return fail. If only query, it will return old
// * record with different transaction and new record with same transaction.
// * @return -1: Failed to read values due to no right version stamp for
// * primary or locked; 0: Passed to read values with all fields. 1: Passed to
// read part of values due to same of fields saved in overflow file.
// */
// int LeafRecord::GetListValue(VectorDataValue &vctVal, uint64_t verStamp,
//                             Transaction *tran, bool bQuery) const {
//  _indexTree->CloneValues(vctVal);
//  bool bPri =
//      (_indexTree->GetHeadPage()->ReadIndexType() == IndexType::PRIMARY);
//  if (!bPri) {
//    if (_tran != nullptr && _tran != tran)
//      return -1;
//
//    uint16_t lenPos = TWO_SHORT_LEN + GetKeyLength();
//    uint16_t pos = lenPos + _indexTree->GetValVarLen();
//    uint16_t len = 0;
//
//    for (uint16_t i = 0; i < vctVal.size(); i++) {
//      if (!vctVal[i]->IsFixLength() && !bPri) {
//        len = *((uint16_t *)(_bysVal + lenPos));
//        lenPos += sizeof(uint16_t);
//      }
//
//      pos += vctVal[i]->ReadData(_bysVal + pos, len);
//    }
//
//    return 0;
//  }
//
//  const LeafRecord *lr = nullptr;
//  if (_tran == nullptr || _tran == tran ||
//      _tran->GetTransactionStatus() == TranStatus::CLOSED ||
//      (bQuery && _actionType == ActionType::QUERY_UPDATE)) {
//    lr = this;
//  } else if (tran->GetTransactionStatus() == TranStatus::FAILED ||
//             tran->GetTransactionStatus() == TranStatus::ROLLBACK ||
//             (bQuery && (_actionType == ActionType::INSERT ||
//                         _actionType == ActionType::UPDATE ||
//                         _actionType == ActionType::DELETE))) {
//    lr = this;
//    while (lr->_undoRecords != nullptr) {
//      lr = lr->_undoRecords;
//    }
//    if (lr->_tran != nullptr && lr->_actionType != ActionType::QUERY_UPDATE) {
//      return -1;
//    }
//  } else {
//    return -1;
//  }
//
//  PriValStruct *priStru = lr->GetPriValStruct();
//  Byte ver = 0;
//  for (; ver < priStru->verCount; ver++) {
//    if (priStru->arrStamp[ver] <= verStamp) {
//      break;
//    }
//  }
//
//  if (ver >= priStru->verCount) {
//    return -1;
//  }
//
//  int hr = 0;
//  if (ver == 0 || !priStru->bOvf) {
//    int indexOvfStart = (int)vctVal.size();
//    int pos = 0;
//    int len = 0;
//    if (ver == 0) {
//      indexOvfStart = lr->GetIndexOvfStart();
//      if (indexOvfStart >= vctVal.size()) {
//        indexOvfStart = (int)vctVal.size();
//      } else {
//        hr = 1;
//      }
//      pos = priStru->pageValOffset;
//      len = (priStru->bOvf ? priStru->lenInPage : priStru->arrPageLen[0]);
//    } else {
//      pos = priStru->pageValOffset + priStru->arrPageLen[ver - 1];
//      len = priStru->arrPageLen[ver];
//    }
//
//    if (len == 0) {
//      vctVal.RemoveAll();
//    } else {
//      for (uint16_t i = 0; i < indexOvfStart; i++) {
//        pos += vctVal[i]->ReadData(lr->_bysVal + pos, -1);
//      }
//    }
//  } else {
//    uint64_t offset = priStru->ovfOffset + priStru->arrOvfLen[ver - 1];
//    uint32_t totalLen = priStru->arrOvfLen[ver] - priStru->arrOvfLen[ver - 1];
//
//    PageFile *ovf = _indexTree->GetOverflowFile();
//    ovf->ReadDataValue(vctVal, 0, offset, totalLen);
//  }
//
//  return hr;
//}
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
//
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
