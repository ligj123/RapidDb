#include "LeafRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../dataType/DataValueFactory.h"

namespace storage {
  LeafRecord::LeafRecord(LeafPage* parentPage, Byte* bys) :
    RawRecord(parentPage->GetIndexTree(), parentPage, bys, false),
    _undoRecords(nullptr), _tranId(0) { }

  LeafRecord::LeafRecord(IndexTree* indexTree, Byte* bys) :
    RawRecord(indexTree, nullptr, bys, false), _undoRecords(nullptr), _tranId(0) { }

  LeafRecord::LeafRecord(IndexTree* indexTree, const VectorDataValue& vctKey, Byte* bysPri, uint32_t lenPri,
    ActionType type, uint64_t tranId, LeafRecord* old) :
    RawRecord(indexTree, nullptr, nullptr, true), _undoRecords(old), _tranId(tranId) {
    _actionType = type;
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();

    int i;
    uint16_t lenKey = keyVarNum * sizeof(uint16_t);
    for (i = 0; i < vctKey.size(); i++) {
      lenKey += vctKey[i]->GetPersistenceLength(true);
    }

    if (lenKey > Configure::GetMaxKeyLength()) {
      throw utils::ErrorMsg(CORE_EXCEED_KEY_LENGTH, { std::to_string(lenKey) });
    }

    int totalLen = lenKey + lenPri + sizeof(uint16_t) * 2;
    _bysVal = CachePool::ApplyBys(totalLen);
    *((uint16_t*)_bysVal) = totalLen;
    *((uint16_t*)(_bysVal + sizeof(uint16_t))) = lenKey;

    uint16_t pos = (2 + keyVarNum) * sizeof(uint16_t);
    uint16_t lenPos = 2 * sizeof(uint16_t);

    for (i = 0; i < vctKey.size(); i++) {
      uint16_t len = vctKey[i]->WriteData(_bysVal + pos, true);
      if (!vctKey[i]->IsFixLength()) {
        *((uint16_t*)(_bysVal + lenPos)) = len;
        lenPos += sizeof(uint16_t);
      }

      pos += len;
    }

    pos = 2 * sizeof(uint16_t) + lenKey;
    std::memcpy(_bysVal + pos, bysPri, lenPri);
  }

  LeafRecord::LeafRecord(IndexTree* indexTree, const VectorDataValue& vctKey, const VectorDataValue& vctVal,
    uint64_t recStamp, ActionType type, uint64_t tranId, LeafRecord* old) :
    RawRecord(indexTree, nullptr, nullptr, true), _undoRecords(old), _tranId(tranId)
  {
    _actionType = type;
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();

    int i;
    uint16_t lenKey = keyVarNum * sizeof(uint16_t);
    for (i = 0; i < vctKey.size(); i++) {
      lenKey += vctKey[i]->GetPersistenceLength(true);
    }

    if (lenKey > Configure::GetMaxKeyLength()) {
      throw utils::ErrorMsg(CORE_EXCEED_KEY_LENGTH, { std::to_string(lenKey) });
    }

    uint16_t lenVal = 0;
    uint16_t max_lenVal = (uint16_t)(Configure::GetMaxRecordLength() - lenKey - sizeof(uint16_t) * 2);
    for (i = 0; i < vctVal.size(); i++) {
      lenVal += vctVal[i]->GetPersistenceLength(false);
      if (lenVal > max_lenVal) {
        lenVal -= vctVal[i]->GetPersistenceLength(false);
        break;
      }
    }

    uint16_t indexOvfStart = (i < vctVal.size() ? i : (uint16_t)vctVal.size());
    uint32_t sizeOverflow = 0;
    if (i < vctVal.size()) {
      for (; i < vctVal.size(); i++) {
        sizeOverflow += vctVal[i]->GetPersistenceLength(false);
      }
    }

    PriValStruct* oldValStru = (old == nullptr ? nullptr : old->GetPriValStruct());
    uint32_t snapLen = (old == nullptr ? 0 : old->GetSnapshotLength());
    uint64_t lver = _indexTree->GetHeadPage()->GetLastVersionStamp();
    bool bOldAll = (oldValStru == nullptr || oldValStru->arrStamp[0] < lver);
    Byte rsCount = (old == nullptr ? 1 : (oldValStru->verCount + (bOldAll ? 1 : 0)));

    uint16_t lenAttr = 1 + sizeof(uint64_t) * rsCount;
    if (sizeOverflow > 0 || lenVal + snapLen > max_lenVal) {
      lenAttr += sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) * rsCount;
    }
    else {
      lenAttr += sizeof(uint16_t) * rsCount;
    }

    _priStru = new PriValStruct;
    _priStru->bLastOvf = (sizeOverflow > 0);
    _priStru->bOvf = (sizeOverflow > 0 || (lenAttr + lenVal + snapLen > max_lenVal && snapLen > 0));
    _priStru->verCount = rsCount;

    int totalLen = lenKey + sizeof(uint16_t) * 2 + lenAttr + lenVal;
    if (!_priStru->bOvf) totalLen += snapLen;

    _bysVal = CachePool::ApplyBys(totalLen);
    *((uint16_t*)_bysVal) = totalLen;
    *((uint16_t*)(_bysVal + sizeof(uint16_t))) = lenKey;

    uint16_t pos = (2 + keyVarNum) * sizeof(uint16_t);
    uint16_t lenPos = 2 * sizeof(uint16_t);

    for (i = 0; i < vctKey.size(); i++) {
      uint16_t len = vctKey[i]->WriteData(_bysVal + pos, true);
      if (!vctKey[i]->IsFixLength()) {
        *((uint16_t*)(_bysVal + lenPos)) = len;
        lenPos += sizeof(uint16_t);
      }

      pos += len;
    }

    _priStru->pageValOffset = pos + lenAttr;

    _bysVal[pos] = (_priStru->bLastOvf ? LAST_RECORD_OVERFLOW : 0)
      + (_priStru->bOvf ? RECORD_OVERFLOW : 0) + _priStru->verCount;
    pos++;

    _priStru->arrStamp = (uint64_t*)&_bysVal[pos];
    utils::UInt64ToBytes(recStamp, _bysVal + pos, false);
    pos += sizeof(uint64_t) * rsCount;

    if (rsCount > 1) {
      std::memcpy(&_priStru->arrStamp[1], &oldValStru->arrStamp[bOldAll ? 0 : 1], sizeof(uint64_t) * (rsCount - 1));
    }

    if (!_priStru->bOvf) {
      _priStru->arrPageLen = (uint16_t*)&_bysVal[pos];
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
        std::memcpy(_bysVal + _priStru->pageValOffset + _priStru->arrPageLen[i - 1],
          old->_bysVal + oldValStru->pageValOffset + oldStart,
          _priStru->arrPageLen[i] - _priStru->arrPageLen[i - 1]);
        oldStart = oldValStru->arrPageLen[oldIndex];
        oldIndex++;
      }
    }
    else {
      _priStru->indexOvfStart = indexOvfStart;
      _priStru->lenInPage = lenVal;
      _priStru->arrOvfLen = (uint32_t*)&_bysVal[pos + sizeof(uint64_t) * 2];
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
        }
        else {
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

      PageFile* ovf = indexTree->GetOverflowFile();

      if (oldValStru == nullptr || !oldValStru->bOvf || oldValStru->ovfRange < sizeOverflow + snapLen) {
        _priStru->ovfRange = sizeOverflow + snapLen + (uint32_t)Configure::GetDiskClusterSize();
        _priStru->ovfRange -= _priStru->ovfRange % (uint32_t)Configure::GetDiskClusterSize();
        _priStru->ovfOffset = ovf->GetOffsetAddLength(_priStru->ovfRange);
      }
      else {
        _priStru->ovfRange = oldValStru->ovfRange;
        _priStru->ovfOffset = oldValStru->ovfOffset;
      }

      utils::UInt64ToBytes(_priStru->ovfOffset, _bysVal + pos, false);
      pos += sizeof(uint64_t);
      utils::UInt32ToBytes(_priStru->ovfRange, _bysVal + pos, false);
      pos += sizeof(uint32_t);
      utils::UInt16ToBytes(indexOvfStart, _bysVal + pos, false);
      pos += sizeof(uint16_t);
      utils::UInt16ToBytes(lenVal, _bysVal + pos, false);
      pos += sizeof(uint16_t);

      pos += sizeof(uint32_t) * rsCount;

      for (int i = 0; i < indexOvfStart; i++) {
        pos += vctVal[i]->WriteData(_bysVal + pos, false);
      }

      ovf->WriteDataValue(vctVal, indexOvfStart, _priStru->ovfOffset);

      if (rsCount > 1) {
        if (!oldValStru->bOvf) {
          uint32_t valStart = oldValStru->pageValOffset + (bOldAll ? 0 : oldValStru->arrPageLen[0]);
          uint32_t valLen = oldValStru->pageValOffset + oldValStru->arrPageLen[oldValStru->verCount - 1] - valStart;
          ovf->WritePage(_priStru->ovfOffset + sizeOverflow, (char*)(old->_bysVal + valStart), valLen);
        }
        else {
          uint64_t offset = _priStru->ovfOffset + sizeOverflow;
          if (bOldAll) {
            ovf->WritePage(offset, (char*)(old->_bysVal + oldValStru->pageValOffset),
              oldValStru->lenInPage);
            offset += oldValStru->lenInPage;
          }

          uint64_t valStart = oldValStru->ovfOffset + (bOldAll ? 0 : oldValStru->arrOvfLen[0]);
          uint64_t valLen = oldValStru->arrOvfLen[oldValStru->verCount - 1] - valStart;

          ovf->MoveOverflowData(valStart, offset, (uint32_t)valLen);
        }
      }
    }
  }

  LeafRecord::~LeafRecord() {
    CleanUndoRecord();
    if (_priStru != nullptr) delete _priStru;
  }

  void LeafRecord::CleanUndoRecord() {
    _tranId = UINT64_MAX;
    LeafRecord* rec = _undoRecords;
    while (rec != nullptr) {
      LeafRecord* next = rec->_undoRecords;
      rec->ReleaseRecord();
      rec = next;
    }

    _undoRecords = nullptr;
  }

  void LeafRecord::GetListKey(VectorDataValue& vctKey) const {
    _indexTree->CloneKeys(vctKey);
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
    uint16_t pos = (2 + keyVarNum) * sizeof(uint16_t);
    uint16_t lenPos = 2 * sizeof(uint16_t);
    uint16_t len = 0;

    for (uint16_t i = 0; i < vctKey.size(); i++) {
      if (!vctKey[i]->IsFixLength()) {
        len = *((uint16_t*)(_bysVal + lenPos));
        lenPos += sizeof(uint16_t);
      }

      pos += vctKey[i]->ReadData(_bysVal + pos, len);
    }
  }

  int LeafRecord::GetListValue(VectorDataValue& vctVal, uint64_t verStamp) const {
    _indexTree->CloneValues(vctVal);
    bool bPri = (_indexTree->GetHeadPage()->ReadIndexType() == IndexType::PRIMARY);
    if (!bPri) {
      uint16_t valVarNum = _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
      uint16_t lenPos = 2 * sizeof(uint16_t) + GetKeyLength();
      uint16_t pos = lenPos + valVarNum * sizeof(uint16_t);
      uint16_t len = 0;

      for (uint16_t i = 0; i < vctVal.size(); i++) {
        if (!vctVal[i]->IsFixLength() && !bPri) {
          len = *((uint16_t*)(_bysVal + lenPos));
          lenPos += sizeof(uint16_t);
        }

        pos += vctVal[i]->ReadData(_bysVal + pos, len);
      }

      return 0;
    }

    PriValStruct* priStru = GetPriValStruct();
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
        indexOvfStart = GetIndexOvfStart();
        if (indexOvfStart >= vctVal.size()) {
          indexOvfStart = (int)vctVal.size();
        }
        else {
          hr = 1;
        }
        pos = priStru->pageValOffset;
        len = (priStru->bOvf ? priStru->lenInPage : priStru->arrPageLen[0]);
      }
      else {
        pos = priStru->pageValOffset + priStru->arrPageLen[ver - 1];
        len = priStru->arrPageLen[ver];
      }

      if (len == 0) {
        vctVal.RemoveAll();
      }
      else {
        for (uint16_t i = 0; i < indexOvfStart; i++) {
          pos += vctVal[i]->ReadData(_bysVal + pos, -1);
        }
      }
    }
    else {
      uint64_t offset = priStru->ovfOffset + priStru->arrOvfLen[ver - 1];
      uint32_t totalLen = priStru->arrOvfLen[ver] - priStru->arrOvfLen[ver - 1];

      PageFile* ovf = _indexTree->GetOverflowFile();
      ovf->ReadDataValue(vctVal, 0, offset, totalLen);
    }

    return hr;
  }

  void LeafRecord::GetListOverflow(VectorDataValue& vctVal) const {
    uint16_t indexOvfStart = GetIndexOvfStart();
    if (indexOvfStart >= vctVal.size()) return;

    PriValStruct* priStru = GetPriValStruct();
    PageFile* ovf = _indexTree->GetOverflowFile();
    ovf->ReadDataValue(vctVal, indexOvfStart, priStru->ovfOffset, priStru->arrOvfLen[0]);
  }

  int LeafRecord::CompareTo(const LeafRecord& lr) const {
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
    int rt = utils::BytesCompare(_bysVal + (2 + keyVarNum) * sizeof(uint16_t), GetKeyLength() - keyVarNum * sizeof(uint16_t),
      lr._bysVal + (2 + keyVarNum) * sizeof(uint16_t), lr.GetKeyLength() - keyVarNum * sizeof(uint16_t));
    if (rt != 0) {
      return rt;
    }

    return utils::BytesCompare(_bysVal + GetKeyLength() + 2 * sizeof(uint16_t), GetValueLength(),
      lr._bysVal + lr.GetKeyLength() + 2 * sizeof(uint16_t), lr.GetValueLength());
  }

  int LeafRecord::CompareKey(const RawKey& key) const {
    int keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();

    return utils::BytesCompare(_bysVal + (2 + keyVarNum) * sizeof(uint16_t),
      GetKeyLength() - keyVarNum * sizeof(uint16_t),
      key.GetBysVal(), key.GetLength());
  }

  int LeafRecord::CompareKey(const LeafRecord& lr) const {
    int keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
    return utils::BytesCompare(_bysVal + (2 + keyVarNum) * sizeof(uint16_t),
      GetKeyLength() - keyVarNum * sizeof(uint16_t),
      lr.GetBysValue() + (2 + keyVarNum) * sizeof(uint16_t),
      lr.GetKeyLength() - keyVarNum * sizeof(uint16_t));
  }

  RawKey* LeafRecord::GetKey() const {
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
    return new RawKey(_bysVal + (2 + keyVarNum) * sizeof(uint16_t), GetKeyLength() - keyVarNum * sizeof(uint16_t));
  }

  RawKey* LeafRecord::GetPrimayKey() const {
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
    int start = GetKeyLength() + sizeof(uint16_t) * (2 + keyVarNum);
    int len = GetTotalLength() - start;
    Byte* buf = CachePool::ApplyBys(len);
    memcpy(buf, _bysVal + start, len);
    return new RawKey(buf, len, true);
  }

  uint32_t LeafRecord::GetSnapshotLength() const {
    uint64_t lver = _indexTree->GetHeadPage()->GetLastVersionStamp();
    PriValStruct* priStru = GetPriValStruct();

    if (priStru->arrStamp[0] <= lver) {
      if (priStru->bOvf) {
        return priStru->lenInPage + priStru->arrOvfLen[priStru->verCount - 1];
      }
      else {
        return priStru->arrPageLen[priStru->verCount - 1];
      }
    }
    else {
      if (priStru->bOvf) {
        return priStru->arrOvfLen[priStru->verCount - 1] - priStru->arrOvfLen[0];
      }
      else {
        return priStru->arrPageLen[priStru->verCount - 1] - priStru->arrPageLen[0];
      }
    }
  }

  std::ostream& operator<< (std::ostream& os, const LeafRecord& lr) {
    VectorDataValue vctKey;
    lr.GetListKey(vctKey);
    os << "TotalLen=" << lr.GetTotalLength() << "  Keys=";
    for (IDataValue* dv : vctKey) {
      os << *dv << "; ";
    }

    VectorDataValue vctVal;
    lr.GetListValue(vctVal);
    os << "  Values=";
    for (IDataValue* dv : vctVal) {
      os << *dv << "; ";
    }

    return os;
  }
}
