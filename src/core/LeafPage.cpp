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
const uint16_t IndexPage::MAX_DATA_LENGTH_LEAF =
    (uint16_t)(CACHE_PAGE_SIZE - LeafPage::DATA_BEGIN_OFFSET - UI32_LEN);

LeafPage::LeafPage(IndexTree *indexTree, PageID pageId, PageID parentPageId)
    : IndexPage(indexTree, pageId, 0, parentPageId, PageType::LEAF_PAGE),
      _prevPageId(PAGE_NULL_POINTER), _nextPageId(PAGE_NULL_POINTER) {
  _vctRecord.reserve(256);
}

LeafPage::LeafPage(IndexTree *indexTree, PageID pageId)
    : IndexPage(indexTree, pageId, PageType::LEAF_PAGE),
      _prevPageId(PAGE_NULL_POINTER), _nextPageId(PAGE_NULL_POINTER) {
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
    ((LeafRecord *)lr)->DecRef();
  }

  _vctRecord.clear();
}

bool LeafPage::SaveRecords() {
  if (_totalDataLength > MAX_DATA_LENGTH_LEAF || _tranCount > 0)
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

  return true;
}

void LeafPage::InsertRecord(LeafRecord *lr, int32_t pos, bool incRef) {
  assert(pos >= 0);

  if (incRef) {
    lr->AddRef();
  }
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
  _bRecordUpdate = true;
  _indexTree->GetHeadPage()->GetAndIncTotalRecordCount();
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
  if (lr->IsTransaction()) {
    _tranCount++;
  }
  _bDirty = true;
  _bRecordUpdate = true;
  _indexTree->GetHeadPage()->GetAndIncTotalPageCount();

  return true;
}

LeafRecord *LeafPage::GetRecord(int32_t pos) {
  assert(pos >= 0 && pos < (int32_t)_recordNum);
  if (_vctRecord.size() > 0) {
    return GetVctRecord(pos)->AddRef();
  } else {
    uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + pos * UI16_LEN);
    return new LeafRecord(this, _bysPage + offset);
  }
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
      hr = bUnique ? GetVctRecord(middle)->CompareKey(rr)
                   : GetVctRecord(middle)->CompareTo(rr);
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
                 ? GetVctRecord(middle - 1)->CompareKey(key) == 0
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
        return middle;
      } else {
        if (middle > start &&
            (_vctRecord.size() > 0
                 ? GetVctRecord(middle - 1)->CompareKey(rr) == 0
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

/**
 * @brief This method is static, it will be called When a statement rollback. It
 * will rollback all inserted LeafRecords
 * @param vctRec The vector to save leaf records
 * @param endPos The end position to rollback records, include this position.
 */
void LeafPage::RollbackLeafRecords(const MVector<LeafRecord *> &vctRec,
                                   int64_t endPos) {
  assert(vctRec.size() > (size_t)endPos);
  for (int64_t i = endPos; i > 0; i--) {
    LeafRecord *lr = vctRec[i];
    LeafPage *page = (LeafPage *)lr->GetParentPage();
    if (page == nullptr)
      continue;

    while (true) {
      page->WriteLock();
      if (lr->GetParentPage() == page) {
        break;
      }
      page->WriteUnlock();
      page = (LeafPage *)lr->GetParentPage();
    }

    bool bFind;
    int32_t pos = page->SearchRecord(*lr, bFind);
    assert(bFind);
    LeafRecord *undoRec = lr->GetUndoRecord();
    if (undoRec != nullptr) {
      page->_vctRecord[pos] = undoRec;
      lr->SaveUndoRecord(nullptr);
      page->_totalDataLength +=
          undoRec->GetTotalLength() - lr->GetTotalLength();
      if (!undoRec->IsTransaction())
        page->_tranCount--;
    } else {
      page->_vctRecord.erase(page->_vctRecord.begin() + pos);
      page->_recordNum--;
      page->_tranCount--;
    }

    lr->SetParentPage(nullptr);
  }
}
} // namespace storage
