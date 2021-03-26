#include "BranchPage.h"
#include "BranchRecord.h"
#include "../pool/StoragePool.h"
#include "IndexTree.h"
#include  <shared_mutex>

namespace storage {
const uint16_t BranchPage::DATA_BEGIN_OFFSET = 16;
const uint16_t BranchPage::MAX_DATA_LENGTH =
(uint16_t)(Configure::GetCachePageSize() - DATA_BEGIN_OFFSET - sizeof(uint64_t));

BranchPage::~BranchPage() {
  CleanRecords();
}

void BranchPage::CleanRecords() {
  for (RawRecord* rr : _vctRecord) {
    ((BranchRecord*)rr)->ReleaseRecord();;
  }

  _vctRecord.clear();
}

void BranchPage::ReadPage() {
  CleanRecords();
  CachePage::ReadPage();

  _recordNum = ReadShort(NUM_RECORD_OFFSET);
  _totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadLong(PARENT_PAGE_POINTER_OFFSET);
  _dtPageLastUpdate = GetMsFromEpoch();
}

void BranchPage::LoadRecords() {
  if (_vctRecord.size() > 0) CleanRecords();

  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    BranchRecord* rr = new BranchRecord(this, _bysPage + *((uint16_t*)(_bysPage + pos)));
    _vctRecord.push_back(rr);
    pos += sizeof(uint16_t);
  }
}

bool BranchPage::SaveRecords() {
  if (_totalDataLength > MAX_DATA_LENGTH) return false;

  unique_lock<SpinMutex> lock(_pageLock, try_to_lock);
  if (!lock.owns_lock()) return false;

  if (_bRecordUpdate) {
    Byte* tmp = _bysPage;
    _bysPage = CachePool::ApplyPage();
    uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * sizeof(uint16_t));

    for (int i = 0; i < _vctRecord.size(); i++) {
      WriteShort(DATA_BEGIN_OFFSET + sizeof(uint16_t) * i, pos);
      BranchRecord* rr = (BranchRecord*)_vctRecord[i];
      pos += rr->SaveData(_bysPage + pos);
      rr->ReleaseRecord();
    }

    _vctRecord.clear();
    _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
    CachePool::ReleasePage(tmp);
  }

  WriteLong(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
  WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
  WriteShort(NUM_RECORD_OFFSET, _recordNum);
  _bRecordUpdate = false;
  _bDirty = true;

  StoragePool::WriteCachePage(this);
  return true;
}

BranchRecord* BranchPage::DeleteRecord(uint16_t index) {
  assert(index >= 0 && index < _recordNum);
  if (_vctRecord.size() == 0) LoadRecords();

  BranchRecord* br = (BranchRecord*)_vctRecord[index];
  _vctRecord.erase(_vctRecord.begin() + index);
  _totalDataLength -= br->GetTotalLength();
  _recordNum--;
  _bRecordUpdate = true;
  _bDirty = true;
  return br;
}

BranchRecord* BranchPage::DeleteRecord(const BranchRecord& rr) {
  if (_vctRecord.size() == 0) {
    LoadRecords();
  }

  bool bFind;
  uint32_t index = SearchRecord(rr, bFind);
  assert(bFind);
  BranchRecord* br = (BranchRecord*)_vctRecord[index];
  _vctRecord.erase(_vctRecord.begin() + index);
  _totalDataLength -= br->GetTotalLength();
  _recordNum--;
  _bRecordUpdate = true;
  _bDirty = true;
  return br;
}

void BranchPage::InsertRecord(BranchRecord*& rr, int32_t pos) {
  if (_recordNum > 0 && _vctRecord.size() == 0) LoadRecords();
  if (pos < 0) {
    bool bFind;
    pos = SearchRecord(*rr, bFind);
  } else if (pos > _recordNum) {
    pos = _recordNum;
  }

  _totalDataLength += rr->GetTotalLength() + sizeof(uint16_t);
  rr->SetParentPage(this);
  _vctRecord.insert(_vctRecord.begin() + pos, rr);
  _recordNum++;
  _bDirty = true;
  _bRecordUpdate = true;
  rr = nullptr;
}

bool BranchPage::AddRecord(BranchRecord*& rr) {
  if (_totalDataLength > MAX_DATA_LENGTH * LOAD_FACTOR
    || _totalDataLength + rr->GetTotalLength() + sizeof(uint16_t) > MAX_DATA_LENGTH) {
    return false;
  }

  _totalDataLength += rr->GetTotalLength() + sizeof(uint16_t);
  rr->SetParentPage(this);
  _vctRecord.push_back(rr);
  _recordNum++;
  _bDirty = true;
  _bRecordUpdate = true;
  rr = nullptr;

  return true;
}

bool BranchPage::RecordExist(const RawKey& key) const {
  if (_recordNum == 0) {
    return false;
  }

  bool bFind;
  SearchKey(key, bFind);
  return bFind;
}

int32_t BranchPage::SearchRecord(const BranchRecord& rr, bool& bFind) const {
  if (_recordNum == 0) {
    bFind = false;
    return 0;
  }

  bFind = true;
  int32_t start = 0;
  int32_t end = _recordNum - 1;
  int hr = (_vctRecord.size() > 0 ? GetVctRecord(end)->CompareTo(rr) : CompareTo(end, rr));
  if (hr == 0) return end;
  else if (hr < 0) {
    bFind = false;
    return end + 1;
  }

  end--;
  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int middle = (start + end) / 2;
    hr = (_vctRecord.size() > 0 ? GetVctRecord(middle)->CompareTo(rr) : CompareTo(middle, rr));
    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      return middle;
    }
  }
}

int32_t BranchPage::SearchKey(const RawKey& key, bool& bFind) const {
  if (_recordNum == 0) {
    bFind = false;
    return 0;
  }

  bool bUnique = (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
  int32_t start = 0;
  int32_t end = _recordNum - 1;
  int hr = (_vctRecord.size() > 0 ? GetVctRecord(end)->CompareKey(key) : CompareTo(end, key));
  if (hr < 0) {
    bFind = false;
    return end + 1;
  }
  if (hr == 0 && bUnique) {
    bFind = true;
    return end;
  }

  end--;
  bFind = true;

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int32_t middle = (start + end) / 2;
    hr = (_vctRecord.size() > 0 ? GetVctRecord(middle)->CompareKey(key) : CompareTo(middle, key));
    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (!bUnique && middle > start && (_vctRecord.size() > 0 ?
        GetVctRecord(middle - 1)->CompareKey(key) == 0 : CompareTo(middle - 1, key) == 0)) {
        end = middle - 1;
      } else {
        return middle;
      }
    }
  }
}

int BranchPage::CompareTo(uint32_t recPos, const BranchRecord& rr) const {
  uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * SHORT_LEN);
  uint32_t lenKey = ReadShort(start + SHORT_LEN);

  int rt = utils::BytesCompare(_bysPage + start + _indexTree->GetKeyOffset(),
    lenKey - _indexTree->GetKeyVarLen(),
    rr.GetBysValue() + _indexTree->GetKeyOffset(),
    rr.GetKeyLength() - _indexTree->GetKeyVarLen());

  if (rt != 0) {
    return rt;
  }

  if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) {
    return 0;
  }

  int lenTotal = ReadShort(start);
  int lenVal = lenTotal - lenKey - BranchRecord::PAGE_ID_LEN - TWO_SHORT_LEN;
  return utils::BytesCompare(_bysPage + start + lenKey + TWO_SHORT_LEN, lenVal,
    rr.GetBysValue() + rr.GetKeyLength() + TWO_SHORT_LEN, rr.GetValueLength());
}

int BranchPage::CompareTo(uint32_t recPos, const RawKey& key) const {
  uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
  uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * SHORT_LEN);
  uint32_t lenKey = ReadShort(start + SHORT_LEN);

  return utils::BytesCompare(_bysPage + start + _indexTree->GetKeyOffset(),
    lenKey - _indexTree->GetKeyVarLen(),
    key.GetBysVal(), key.GetLength());
}

BranchRecord* BranchPage::GetRecordByPos(int32_t pos, bool bAutoLast) {
  assert(_recordNum > 0 && pos >= 0);
  assert(bAutoLast || pos < _recordNum);
  if (bAutoLast && pos >= _recordNum) {
    pos = _recordNum - 1;
  }

  if (_vctRecord.size() == 0) {
    uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + pos * sizeof(uint16_t));
    return new BranchRecord(this, _bysPage + offset);
  } else {
    return GetVctRecord(pos)->ReferenceRecord();
  }
}
}
