#include "LeafPage.h"
#include "../config/ErrorID.h"
#include "../pool/PageDividePool.h"
#include "../pool/StoragePool.h"
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
const uint16_t LeafPage::MAX_DATA_LENGTH_LEAF =
    (uint16_t)(CACHE_PAGE_SIZE - LeafPage::DATA_BEGIN_OFFSET - UI32_LEN);

LeafPage::LeafPage(IndexTree *indexTree, PageID pageId, PageID parentPageId)
    : IndexPage(indexTree, pageId, 0, parentPageId, PageType::LEAF_PAGE),
      _prevPageId(HeadPage::PAGE_NULL_POINTER),
      _nextPageId(HeadPage::PAGE_NULL_POINTER) {
  _vctRecord.reserve(256);
}

LeafPage::LeafPage(IndexTree *indexTree, PageID pageId)
    : IndexPage(indexTree, pageId, PageType::LEAF_PAGE),
      _prevPageId(HeadPage::PAGE_NULL_POINTER),
      _nextPageId(HeadPage::PAGE_NULL_POINTER) {
  _vctRecord.reserve(256);
}

LeafPage::~LeafPage() { CleanRecord(); }

void LeafPage::Init() {
  assert(!_bDirty);
  CleanRecord();
  IndexPage::Init();
  _prevPageId = ReadInt(PREV_PAGE_POINTER_OFFSET);
  _nextPageId = ReadInt(NEXT_PAGE_POINTER_OFFSET);
}

void LeafPage::LoadRecords() {
  CleanRecord();

  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    LeafRecord *rr =
        new LeafRecord(this, _bysPage + *((uint16_t *)(_bysPage + pos)));
    _vctRecord.push_back(rr);
    pos += sizeof(uint16_t);
  }
}

void LeafPage::CleanRecord() {
  for (RawRecord *lr : _vctRecord) {
    ((LeafRecord *)lr)->ReleaseRecord();
  }

  _vctRecord.clear();
}

bool LeafPage::SaveRecords() {
  if (_totalDataLength > MAX_DATA_LENGTH_LEAF)
    return false;

  unique_lock<SpinMutex> lock(_pageLock, try_to_lock);
  if (!lock.owns_lock())
    return false;

  if (_bRecordUpdate) {
    Byte *tmp = _bysPage;
    _bysPage = CachePool::ApplyPage();
    uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * UI16_LEN);
    _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
    _bysPage[PAGE_BEGIN_END_OFFSET] = tmp[PAGE_BEGIN_END_OFFSET];
    int refCount = 0;
    uint16_t index = 0;

    for (uint16_t i = 0; i < _vctRecord.size(); i++) {
      LeafRecord *lr = (LeafRecord *)_vctRecord[i];
      if (_absoBuf != nullptr && !lr->IsSole())
        refCount++;

      if (lr->GetTotalLength() == 0) {
        continue;
      }

      WriteShort(DATA_BEGIN_OFFSET + UI16_LEN * index, pos);
      pos += lr->SaveData(_bysPage + pos);
      index++;
    }

    CleanRecord();
    if (_absoBuf == nullptr || _absoBuf->IsDiffBuff(tmp))
      CachePool::Release(tmp, (uint32_t)Configure::GetCachePageSize());
    if (refCount > 0)
      _absoBuf->ReleaseCount(refCount);

    _absoBuf = nullptr;
  }

  WriteInt(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
  WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
  WriteInt(PREV_PAGE_POINTER_OFFSET, _prevPageId);
  WriteInt(NEXT_PAGE_POINTER_OFFSET, _nextPageId);
  WriteShort(NUM_RECORD_OFFSET, _recordNum);
  _bRecordUpdate = false;
  _bDirty = true;

  StoragePool::WriteCachePage(this);
  return true;
}

ErrorMsg *LeafPage::InsertRecord(LeafRecord *lr, int32_t pos) {
  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  bool bUnique =
      (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
  if (pos < 0) {
    bool bFind;
    if (bUnique) {
      pos = SearchKey(*lr, bFind);
    } else {
      pos = SearchRecord(*lr, bFind);
    }

    if (bFind) {
      return new ErrorMsg(CORE_REPEATED_RECORD, {});
    }
  } else if (pos > (int32_t)_recordNum) {
    pos = _recordNum;
  }

  _totalDataLength += lr->GetTotalLength() + UI16_LEN;
  lr->SetParentPage(this);
  _vctRecord.insert(_vctRecord.begin() + pos, lr);
  _recordNum++;
  _bDirty = true;
  _bRecordUpdate = true;
  _indexTree->GetHeadPage()->GetAndIncTotalRecordCount();
  PageDividePool::AddCachePage(this);

  return nullptr;
}

bool LeafPage::AddRecord(LeafRecord *lr) {
  if (_totalDataLength > MAX_DATA_LENGTH_LEAF * LOAD_FACTOR / 100U ||
      _totalDataLength + lr->GetTotalLength() + UI16_LEN >
          (uint32_t)MAX_DATA_LENGTH_LEAF) {
    return false;
  }

  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  _totalDataLength += lr->GetTotalLength() + sizeof(uint16_t);
  lr->SetParentPage(this);
  _vctRecord.push_back(lr);
  _recordNum++;
  _bDirty = true;
  _bRecordUpdate = true;
  _indexTree->GetHeadPage()->GetAndIncTotalPageCount();

  return true;
}

void LeafPage::UpdateRecord(int32_t szChange, bool bRemove) {
  if (bRemove) {
    _recordNum--;
  }

  _totalDataLength += szChange;
}

LeafRecord *LeafPage::GetRecord(const RawKey &key) {
  if (_recordNum == 0) {
    return nullptr;
  }

  bool bFind;
  int32_t index = SearchKey(key, bFind);
  if (!bFind) {
    return nullptr;
  }

  if (_vctRecord.size() == 0) {
    int pos = ReadShort(DATA_BEGIN_OFFSET + index * sizeof(uint16_t));
    return new LeafRecord(this, _bysPage + pos);
  } else {
    return ((LeafRecord *)_vctRecord[index])->ReferenceRecord();
  }
}

bool LeafPage::GetRecords(const RawKey &key, VectorLeafRecord &vct,
                          bool bFromZero) {
  if (vct.size() > 0)
    vct.RemoveAll();
  if (_recordNum == 0)
    return true;

  bool bFind;
  uint32_t pos = 0;
  if (!bFromZero) {
    pos = SearchKey(key, bFind);
    if (!bFind)
      return false;
  }

  uint32_t end = SearchKey(key, bFind, false, pos);
  vct.reserve(end - pos);

  if (_vctRecord.size() == 0) {
    for (uint16_t i = pos; i < end; i++) {
      uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + i * UI16_LEN);
      vct.push_back(new LeafRecord(this, _bysPage + offset));
    }
  } else {
    for (uint32_t i = pos; i < end; i++) {
      LeafRecord *rr = ((LeafRecord *)_vctRecord[i])->ReferenceRecord();
      vct.push_back(rr);
    }
  }

  return (end == _recordNum);
}

int32_t LeafPage::SearchRecord(const LeafRecord &rr, bool &bFind, bool bInc,
                               int32_t start, int32_t end) {
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
      hr = GetVctRecord(middle)->CompareTo(rr);
    } else {
      hr = CompareTo(middle, rr, false);
    }

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      return bInc ? middle : middle + 1;
    }
  }
}

int32_t LeafPage::SearchKey(const RawKey &key, bool &bFind, bool bInc,
                            int32_t start, int32_t end) {
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
        return bInc ? middle : middle + 1;
      } else {
        if (bInc) {
          if (middle > start &&
              (_vctRecord.size() > 0
                   ? GetVctRecord(middle - 1)->CompareKey(key) == 0
                   : CompareTo(middle - 1, key) == 0)) {
            end = middle - 1;
          } else {
            return middle;
          }
        } else {
          if (middle < end &&
              (_vctRecord.size() > 0
                   ? GetVctRecord(middle + 1)->CompareKey(key) == 0
                   : CompareTo(middle + 1, key) == 0)) {
            start = middle + 1;
          } else {
            return middle + 1;
          }
        }
      }
    }
  }
}

int32_t LeafPage::SearchKey(const LeafRecord &rr, bool &bFind, bool bInc,
                            int32_t start, int32_t end) {
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
      hr = GetVctRecord(middle)->CompareKey(rr);
    } else {
      hr = CompareTo(middle, rr, true);
    }

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (bUnique) {
        return bInc ? middle : middle + 1;
      } else {
        if (bInc) {
          if (middle > start &&
              (_vctRecord.size() > 0
                   ? GetVctRecord(middle - 1)->CompareKey(rr) == 0
                   : CompareTo(middle - 1, rr, true) == 0)) {
            end = middle - 1;
          } else {
            return middle;
          }
        } else {
          if (middle < end &&
              (_vctRecord.size() > 0
                   ? GetVctRecord(middle + 1)->CompareKey(rr) == 0
                   : CompareTo(middle + 1, rr, true) == 0)) {
            start = middle + 1;
          } else {
            return middle + 1;
          }
        }
      }
    }
  }
}

LeafRecord *LeafPage::GetLastRecord() {
  if (_recordNum == 0)
    return nullptr;
  if (_vctRecord.size() > 0) {
    return GetVctRecord(_recordNum - 1)->ReferenceRecord();
  } else {
    uint16_t offset =
        ReadShort(DATA_BEGIN_OFFSET + (_recordNum - 1) * UI16_LEN);
    return new LeafRecord(this, _bysPage + offset);
  }
}

void LeafPage::GetAllRecords(VectorLeafRecord &vct) {
  vct.RemoveAll();
  vct.reserve(_recordNum);
  if (_vctRecord.size() == 0) {
    LoadRecords();
  }

  for (RawRecord *rr : _vctRecord) {
    vct.push_back(((LeafRecord *)rr)->ReferenceRecord());
  }
}

bool LeafPage::FetchRecords(const RawKey *startKey, const RawKey *endKey,
                            const bool bIncLeft, const bool bIncRight,
                            VectorLeafRecord &vct) {
  assert(startKey == nullptr || endKey == nullptr || *startKey <= *endKey);
  if (_vctRecord.size() == 0) {
    LoadRecords();
  }
  int32_t start = 0, end = _recordNum;
  bool bFind;
  if (startKey != nullptr && CompareTo(0, *startKey) < 0) {
    start = SearchKey(*startKey, bFind, bIncLeft);
  }

  if (endKey != nullptr && CompareTo(_recordNum - 1, *endKey) > 0) {
    end = SearchKey(*endKey, bFind, !bIncRight, start);
  }

  vct.RemoveAll();
  vct.reserve(end - start);
  for (; start < end; start++) {
    vct.push_back(((LeafRecord *)_vctRecord[start])->ReferenceRecord());
  }

  return end >= (int32_t)_recordNum;
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
} // namespace storage
