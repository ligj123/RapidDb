#include "BranchPage.h"
#include "../pool/StoragePool.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include <shared_mutex>

namespace storage {
const uint16_t BranchPage::DATA_BEGIN_OFFSET = 12;
const uint16_t IndexPage::MAX_DATA_LENGTH_BRANCH =
    (uint16_t)(Configure::GetIndexPageSize() - BranchPage::DATA_BEGIN_OFFSET -
               sizeof(uint32_t));

void BranchPage::InitParameters() {
  _recordNum = ReadShort(NUM_RECORD_OFFSET);
  _totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadInt(PARENT_PAGE_POINTER_OFFSET);
  _children.resize(_recordNum, nullptr);
}

void BranchPage::LoadRecords() {
  assert(!_bDirty);
  _vctRecord.reserve(_recordNum);

  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    _vctRecord.push_back(
        new BranchPage(_indexTree->GetHeadPage()->GetIndexType(),
                       _bysPage + *((uint16_t *)(_bysPage + pos))));
    pos += sizeof(uint16_t);
  }
}

void BranchPage::ClearRecords() {
  for (auto &br : _vctRecord) {
    delete br;
  }

  _vctRecord.resize(0);
}

bool BranchPage::SaveToBuffer() {
  assert(_bDirty);
  if (_totalDataLength > MAX_DATA_LENGTH_BRANCH)
    return false;

  Byte *tmp = _bysPage;
  _bysPage = CachePool::ApplyPage();
  _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
  _bysPage[PAGE_BEGIN_END_OFFSET] = tmp[PAGE_BEGIN_END_OFFSET];
  WriteInt(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
  WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
  WriteShort(NUM_RECORD_OFFSET, _recordNum);

  uint16_t rec_pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * UI16_LEN);
  uint16_t off_pos = DATA_BEGIN_OFFSET;
  Byte *bys_rec = _bysPage + rec_pos;

  for (int i = 0; i < _vctRecord.size(); i++) {
    WriteShort(off_pos, rec_pos);
    BranchRecord *rr = _vctRecord[i];
    uint16_t sz = rr->SaveData(bys_rec);
    rr->UpdateBysValue(bys_rec);
    bys_rec += sz;
    rec_pos += sz;
    off_pos += UI16_LEN;
  }

  CachePool::ReleasePage(tmp);
  _bDirty = false;
  return true;
}

BranchRecord *BranchPage::DeleteRecord(uint16_t index) {
  assert(index >= 0 && index < _recordNum);
  if (_vctRecord.size() == 0)
    LoadRecords();

  BranchRecord *brDel = _vctRecord[index];
  _totalDataLength -= brDel.GetTotalLength() + UI16_LEN;
  _recordNum--;
  _vctRecord.erase(_vctRecord.begin() + index);
  _bDirty = true;
  return brDel;
}

void BranchPage::InsertRecord(BranchRecord *record, int32_t pos) {
  assert(pos >= 0 && pos <= _recordNum);
  if (_recordNum > 0 && _vctRecord.size() == 0)
    LoadRecords();

  _totalDataLength += record->GetTotalLength() + UI16_LEN;
  _vctRecord.insert(_vctRecord.begin() + pos, record);
  _recordNum++;
  _bDirty = true;
}

bool BranchPage::AddRecord(BranchRecord *rr) {
  if (_totalDataLength > MAX_DATA_LENGTH_BRANCH * LOAD_FACTOR / 100U ||
      _totalDataLength + rr->GetTotalLength() + UI16_LEN >
          MAX_DATA_LENGTH_BRANCH) {
    return false;
  }

  _totalDataLength += rr->GetTotalLength() + UI16_LEN;
  _vctRecord.push_back(rr);
  _recordNum++;
  _bDirty = true;

  return true;
}

bool BranchPage::RecordExist(const RawKey &key) const {
  if (_recordNum == 0) {
    return false;
  }

  bool bFind;
  SearchKey(key, bFind);
  return bFind;
}

int32_t BranchPage::SearchRecord(const BranchRecord &rr, bool &bFind) const {
  bFind = true;
  int32_t start = 0;
  int32_t end = _recordNum - 1;

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int middle = (start + end) / 2;
    int hr = (_vctRecord.size() > 0 ? GetVctRecord(middle)->CompareTo(rr)
                                    : CompareTo(middle, rr));
    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      return middle;
    }
  }
}

int32_t BranchPage::SearchKey(const RawKey &key, bool &bFind) const {
  bool bUnique =
      (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
  int32_t start = 0;
  int32_t end = _recordNum - 1;
  bFind = true;

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int32_t middle = (start + end) / 2;
    int hr = (_vctRecord.size() > 0 ? GetVctRecord(middle)->CompareKey(key)
                                    : CompareTo(middle, key));
    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (!bUnique && middle > start &&
          (_vctRecord.size() > 0
               ? GetVctRecord(middle - 1)->CompareKey(key) == 0
               : CompareTo(middle - 1, key) == 0)) {
        end = middle - 1;
      } else {
        return middle;
      }
    }
  }
}

int BranchPage::CompareTo(uint32_t recPos, const BranchRecord &rr) const {
  assert(recPos < _recordNum);
  uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * UI16_LEN);
  uint32_t lenKey = ReadShort(start + UI16_LEN);

  if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) {
    return BytesCompare(_bysPage + start + _indexTree->GetKeyOffset(),
                        ReadShort(start + UI16_LEN) -
                            _indexTree->GetKeyVarLen(),
                        rr.GetBysValue() + _indexTree->GetKeyOffset(),
                        rr.GetKeyLength() - _indexTree->GetKeyVarLen());
  } else {
    return BytesCompare(_bysPage + start + _indexTree->GetKeyOffset(),
                        ReadShort(start) - _indexTree->GetKeyOffset(),
                        rr.GetBysValue() + _indexTree->GetKeyOffset(),
                        rr.GetTotalLength() - _indexTree->GetKeyOffset());
  }
}

int BranchPage::CompareTo(uint32_t recPos, const RawKey &key) const {
  assert(recPos < _recordNum);
  uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * UI16_LEN);

  return BytesCompare(_bysPage + start + _indexTree->GetKeyOffset(),
                      ReadShort(start + UI16_LEN) - _indexTree->GetKeyVarLen(),
                      key.GetBysVal(), key.GetLength());
}

const BranchRecord &BranchPage::GetRecord(uint32_t pos, bool bAutoLast) {
  assert(_recordNum > 0 && pos >= 0);
  assert(bAutoLast || pos < _recordNum);
  if (bAutoLast && pos >= _recordNum) {
    pos = _recordNum - 1;
  }

  if (_vctRecord.size() == 0) {
    LoadRecords();
  }

  return *GetVctRecord(pos);
}
} // namespace storage
