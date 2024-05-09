#include "LeafPage.h"
#include "../pool/PageDividePool.h"
#include "../pool/StoragePool.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "BranchPage.h"
#include "HeadPage.h"
#include "IndexTree.h"
#include "LeafRecord.h"
#include "PageType.h"

namespace storage {
const uint16_t LeafPage::PREV_PAGE_POINTER_OFFSET = 12;
const uint16_t LeafPage::NEXT_PAGE_POINTER_OFFSET = 16;
const uint16_t LeafPage::DATA_BEGIN_OFFSET = 20;
const uint16_t IndexPage::MAX_DATA_LENGTH_LEAF =
    (uint16_t)(INDEX_PAGE_SIZE - LeafPage::DATA_BEGIN_OFFSET - UI32_LEN);

LeafPage::~LeafPage() { ClearRecords(); }

void LeafPage::InitParameters() {
  assert(!_bDirty);
  _recordNum = ReadShort(NUM_RECORD_OFFSET);
  _totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadInt(PARENT_PAGE_POINTER_OFFSET);

  _tranCount = ReadShort(PAGE_TRAN_COUNT_OFFSET);
  _prevPageId = ReadInt(PREV_PAGE_POINTER_OFFSET);
  _nextPageId = ReadInt(NEXT_PAGE_POINTER_OFFSET);
}

void LeafPage::LoadRecords() {
  assert(!_bDirty);
  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    LeafRecord *lr = new LeafRecord(_indexTree->GetHeadPage()->GetIndexType(),
                                    _bysPage + *((uint16_t *)(_bysPage + pos)));
    _vctRecord.push_back(lr);
    pos += sizeof(uint16_t);
  }
}

bool LeafPage::SaveRecords() {
  assert(_bDirty);
  if (_totalDataLength > MAX_DATA_LENGTH_LEAF || _tranCount > 0)
    return false;

  Byte *tmp = nullptr;
  if (_obsBuf == nullptr) {
    tmp = _bysPage;
    _bysPage = CachePool::ApplyPage();
  } else if (_obsBuf->IsSameBuff(_bysPage)) {
    _bysPage = CachePool::ApplyPage();
  }

  _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
  _bysPage[PAGE_BEGIN_END_OFFSET] = tmp[PAGE_BEGIN_END_OFFSET];

  WriteShort(NUM_RECORD_OFFSET, _recordNum);
  WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
  WriteInt(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
  WriteShort(PAGE_TRAN_COUNT_OFFSET, _tranCount);
  WriteInt(PREV_PAGE_POINTER_OFFSET, _prevPageId);
  WriteInt(NEXT_PAGE_POINTER_OFFSET, _nextPageId);

  uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * UI16_LEN);
  uint16_t off_pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _vctRecord.size(); i++) {
    LeafRecord *lr = (LeafRecord *)_vctRecord[i];
    if (lr->GetTotalLength() == 0) {
      continue;
    }

    WriteShort(off_pos, pos);
    uint16_t sz = lr->SaveData(_bysPage + pos);
    rr.UpdateBysValue(_bysPage + pos);
    pos += sz;
    off_pos += UI16_LEN;
  }

  if (_obsBuf != nullptr) {
    _obsBuf->DecRef();
    _obsBuf = nullptr;
  } else if (tmp != nullptr) {
    CachePool::ReleasePage(tmp);
  }

  _bDirty = false;
  return true;
}

void LeafPage::InsertRecord(LeafRecord *lr, int32_t pos) {
  assert(pos >= 0 && pos <= _recordNum);
  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  _totalDataLength += lr->GetTotalLength() + UI16_LEN;
  lr->SetParentPage(this);
  _vctRecord.insert(_vctRecord.begin() + pos, lr);
  _recordNum++;
  if (lr->IsTransaction()) {
    _tranCount++;
  }
  _bDirty = true;
}

bool LeafPage::AddRecord(LeafRecord *lr) {
  assert(!lr->IsTransaction());
  if (_totalDataLength + lr->GetTotalLength() + UI16_LEN >
      (uint32_t)MAX_DATA_LENGTH_LEAF) {
    return false;
  }

  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  _totalDataLength += lr.GetTotalLength() + sizeof(uint16_t);
  _vctRecord.push_back(lr);
  _recordNum++;
  _bDirty = true;
  return true;
}

const LeafRecord &LeafPage::GetRecord(int32_t pos) {
  assert(pos >= 0 && pos < (int32_t)_recordNum);
  if (_vctRecord.size() == 0) {
    LoadRecords();
  }

  return *_vctRecord[pos];
}

int32_t LeafPage::SearchRecord(const LeafRecord &rr, bool &bFind, int32_t start,
                               int32_t end) {
  bool bUnique =
      (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);

  if (end >= (int32_t)_recordNum)
    end = _recordNum - 1;
  bFind = true;
  int hr;

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int middle = (start + end) / 2;
    if (_vctRecord.size() > 0) {
      hr = bUnique ? _vctRecord[middle]->CompareKey(rr)
                   : _vctRecord[middle]->CompareTo(rr);
    } else {
      hr = CompareTo(middle, rr, bUnique);
    }

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      return middle;
    }
  }
}

int32_t LeafPage::SearchKey(const RawKey &key, bool &bFind, int32_t start,
                            int32_t end) {
  if (end >= (int32_t)_recordNum)
    end = _recordNum - 1;
  bFind = true;
  bool bUnique =
      (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int32_t middle = (start + end) / 2;
    int hr = 0;
    if (_vctRecord.size() > 0) {
      hr = GetVctRecord(middle)->CompareKey(key);
    } else {
      hr = CompareTo(middle, key);
    }

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (bUnique) {
        return middle;
      } else {
        if (middle > start &&
            (_vctRecord.size() > 0
                 ? _vctRecord[middle - 1]->CompareKey(key) == 0
                 : CompareTo(middle - 1, key) == 0)) {
          end = middle - 1;
        } else {
          return middle;
        }
      }
    }
  }
}

int32_t LeafPage::SearchKey(const LeafRecord &rr, bool &bFind, int32_t start,
                            int32_t end) {
  if (end >= (int32_t)_recordNum)
    end = _recordNum - 1;
  bFind = true;
  bool bUnique =
      (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int32_t middle = (start + end) / 2;
    int hr = 0;
    if (_vctRecord.size() > 0) {
      hr = _vctRecord[middle].CompareKey(rr);
    } else {
      hr = CompareTo(middle, rr, true);
    }

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (bUnique) {
        return middle;
      } else {
        if (middle > start &&
            (_vctRecord.size() > 0 ? _vctRecord[middle - 1]->CompareKey(rr) == 0
                                   : CompareTo(middle - 1, rr, true) == 0)) {
          end = middle - 1;
        } else {
          return middle;
        }
      }
    }
  }
}

int LeafPage::CompareTo(uint32_t recPos, const RawKey &key) {
  uint16_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * UI16_LEN);
  return BytesCompare(_bysPage + start + _indexTree->GetKeyOffset(),
                      ReadShort(start + UI16_LEN) - _indexTree->GetKeyVarLen(),
                      key.GetBysVal(), key.GetLength());
}

int LeafPage::CompareTo(uint32_t recPos, const LeafRecord &rr, bool key) {
  uint16_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * UI16_LEN);
  if (key) {
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

void LeafPage::ClearRecords() {
  for (auto &lr : _vctRecord) {
    delete lr;
  }

  _vctRecord.resize(0);
}
} // namespace storage
