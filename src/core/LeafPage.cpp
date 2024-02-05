﻿#include "LeafPage.h"
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

LeafPage::LeafPage(IndexTree *indexTree, PageID pageId, PageID parentPageId)
    : IndexPage(indexTree, pageId, 0, parentPageId, PageType::LEAF_PAGE),
      _prevPageId(PAGE_NULL_POINTER), _nextPageId(PAGE_NULL_POINTER) {}

LeafPage::LeafPage(IndexTree *indexTree, PageID pageId)
    : IndexPage(indexTree, pageId, PageType::LEAF_PAGE),
      _prevPageId(PAGE_NULL_POINTER), _nextPageId(PAGE_NULL_POINTER) {}

LeafPage::~LeafPage() { CleanRecord(); }

void LeafPage::LoadVars() {
  assert(!_bDirty);
  IndexPage::LoadVars();
  _prevPageId = ReadInt(PREV_PAGE_POINTER_OFFSET);
  _nextPageId = ReadInt(NEXT_PAGE_POINTER_OFFSET);
}

void LeafPage::LoadRecords() {
  assert(!_bRecordUpdate);
  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    _vctRecord.emplace_back(_indexTree->GetHeadPage()->GetIndexType(),
                            _bysPage + *((uint16_t *)(_bysPage + pos)));
    pos += sizeof(uint16_t);
  }
}

bool LeafPage::SaveRecords() {
  assert(_bDirty || _bRecordUpdate);
  if (_totalDataLength > MAX_DATA_LENGTH_LEAF || _tranCount > 0)
    return false;

  if (_bRecordUpdate) {
    Byte *tmp = _bysPage;
    _bysPage = CachePool::ApplyPage();
    uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * UI16_LEN);
    _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
    _bysPage[PAGE_BEGIN_END_OFFSET] = tmp[PAGE_BEGIN_END_OFFSET];
    uint16_t index = 0;

    for (uint16_t i = 0; i < _vctRecord.size(); i++) {
      LeafRecord *lr = (LeafRecord *)_vctRecord[i];
      if (lr->GetTotalLength() == 0) {
        continue;
      }

      WriteShort(DATA_BEGIN_OFFSET + UI16_LEN * index, pos);
      uint16_t sz = lr->SaveData(_bysPage + pos);
      rr.UpdateBysValue(_bysPage + pos);
      pos += sz;
      index++;
    }

    CachePool::ReleasePage(tmp);
  }

  WriteInt(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
  WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
  WriteInt(PREV_PAGE_POINTER_OFFSET, _prevPageId);
  WriteInt(NEXT_PAGE_POINTER_OFFSET, _nextPageId);
  WriteShort(NUM_RECORD_OFFSET, _recordNum);
  _bRecordUpdate = false;
  _bDirty = false;

  return true;
}

void LeafPage::InsertRecord(LeafRecord &&lr, int32_t pos) {
  assert(pos >= 0);
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
}

bool LeafPage::AddRecord(LeafRecord &&lr) {
  if (_totalDataLength > MAX_DATA_LENGTH_LEAF * LOAD_FACTOR / 100U ||
      _totalDataLength + lr->GetTotalLength() + UI16_LEN >
          (uint32_t)MAX_DATA_LENGTH_LEAF) {
    return false;
  }

  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  _totalDataLength += lr.GetTotalLength() + sizeof(uint16_t);
  _vctRecord.push_back(move(lr));
  _recordNum++;
  if (lr.IsTransaction()) {
    _tranCount++;
  }
  _bDirty = true;
  _bRecordUpdate = true;

  return true;
}

LeafRecord &LeafPage::GetRecord(int32_t pos) {
  assert(pos >= 0 && pos < (int32_t)_recordNum);
  if (_vctRecord.size() == 0) {
    LoadRecords();
  }

  return _vctRecord[pos];
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
      hr = bUnique ? _vctRecord[middle].CompareKey(rr)
                   : _vctRecord[middle].CompareTo(rr);
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
            (_vctRecord.size() > 0 ? _vctRecord[middle - 1].CompareKey(key) == 0
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
            (_vctRecord.size() > 0 ? _vctRecord[middle - 1].CompareKey(rr) == 0
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

bool LeafPage::SplitPage(bool block) {
  assert(block == false);
  BranchRecord brParentOld;
  BranchPage *parentPage;
  int posInParent = 0;
  if (_parentPageId == PAGE_NULL_POINTER) {
    parentPage = (BranchPage *)_indexTree->AllocateNewPage(PAGE_NULL_POINTER,
                                                           GetPageLevel() + 1);
    parentPage->SetBeginPage(true);
    parentPage->SetEndPage(true);

    parentPage->WriteLock();
    _parentPageId = parentPage->GetPageId();
    _indexTree->UpdateRootPage(parentPage);
  } else {
    parentPage = (BranchPage *)_indexTree->GetPage(_parentPageId,
                                                   PageType::BRANCH_PAGE, true);
    if (!parentPage->WriteTryLock()) {
      parentPage->DecRef();
      return false;
    }

    BranchRecord br(_indexTree, _vctRecord[_recordNum - 1], GetPageId());
    bool bFind;
    posInParent = parentPage->SearchRecord(br, bFind);
    if (!bFind)
      posInParent = parentPage->GetRecordNumber() - 1;
    brParentOld = parentPage->DeleteRecord(posInParent);
  }

  // System.out.println("pageDivide");
  // Calc this page's records
  int maxLen = GetMaxDataLength() * LOAD_FACTOR / 100;
  int pos = 0;
  int len = 0;
  int refCount = 0;
  _tranCount = 0;

  for (; pos < _vctRecord.size(); pos++) {
    RawRecord *rr = _vctRecord[pos];
    if (!rr->IsSole())
      refCount++;
    if (rr->IsTransaction())
      _tranCount++;

    len += rr->GetTotalLength() + sizeof(uint16_t);
    if (len > maxLen) {
      if (len > GetMaxDataLength()) {
        len -= rr->GetTotalLength() + sizeof(uint16_t);
      } else {
        pos++;
      }

      break;
    }
  }

  // Split the surplus records to following page
  _totalDataLength = len;
  _recordNum = pos;

  MVector<IndexPage *> vctPage;
  IndexPage *newPage = nullptr;

  len = 0;
  int startPos = pos;
  Byte level = GetPageLevel();
  int tranCount = 0;
  for (int i = pos; i < _vctRecord.size(); i++) {
    RawRecord *rr = _vctRecord[i];
    if (!rr->IsSole())
      refCount++;
    if (rr->IsTransaction())
      tranCount++;

    len += rr->GetTotalLength() + sizeof(uint16_t);

    if (len > maxLen) {
      if (len > GetMaxDataLength()) {
        len -= rr->GetTotalLength() + sizeof(uint16_t);
        i--;
      }

      newPage = _indexTree->AllocateNewPage(parentPage->GetPageId(), level);
      newPage->SetDirty(true);
      newPage->WriteLock();
      newPage->_vctRecord.insert(newPage->_vctRecord.end(),
                                 _vctRecord.begin() + startPos,
                                 _vctRecord.begin() + i + 1);

      newPage->_totalDataLength = len;
      newPage->_recordNum = (int32_t)newPage->_vctRecord.size();
      newPage->_tranCount = tranCount;
      tranCount = 0;
      vctPage.push_back(newPage);
      len = 0;
      startPos = i + 1;
      continue;
    }
  }
} // namespace storage
