#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
const uint16_t BranchPage::DATA_BEGIN_OFFSET = 12;
const uint32_t IndexPage::MAX_DATA_LENGTH_BRANCH =
    (uint32_t)(Configure::GetIndexPageSize() - BranchPage::DATA_BEGIN_OFFSET -
               sizeof(uint32_t));

void BranchPage::InitParameters() {
  _score = 10000;
  _recordNum = ReadShort(NUM_RECORD_OFFSET);
  _committedDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadInt(PARENT_PAGE_POINTER_OFFSET);

  if (_parentPage != nullptr && _parentPage->GetPageId() != _parentPageId)
      [[unlikely]] {
    _parentPageId = _parentPage->GetPageId();
    _bDirty = true;
  }

  LoadRecords();
}

void BranchPage::LoadRecords() {
  assert(!_bDirty && _vctRecord.size() == 0);
  _vctRecord.reserve(_recordNum);

  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    _vctRecord.push_back(
        new BranchRecord(_indexTree->GetHeadPage()->GetIndexType(),
                         _bysPage + *((uint16_t *)(_bysPage + pos))));
    pos += sizeof(uint16_t);
  }
}

void BranchPage::ClearRecords() {
  for (auto &br : _vctRecord) {
    delete br;
  }

  _vctRecord.clear();
}

bool BranchPage::SaveRecords() {
  assert(_bDirty);
  if (GetPageStatus() != PageStatus::VALID ||
      _committedDataLength > MAX_DATA_LENGTH_BRANCH) {
    return false;
  }

  bool bUnlock = false;
  if (GetPageLevel() > _indexTree->GetSplitPageLevel() &&
      _spinLock.owner() != g_threadId) {
    if (!_spinLock.try_lock()) {
      return false;
    }

    bUnlock = true;
  }

  if (_bRecordUpdated) {
    Byte *tmp = _bysPage;
    _bysPage = CachePool::ApplyPage();

    _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
    _bysPage[PAGE_BEGIN_END_OFFSET] = tmp[PAGE_BEGIN_END_OFFSET];

    uint16_t rec_pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * UI16_LEN);
    uint16_t off_pos = DATA_BEGIN_OFFSET;
    Byte *bys_rec = _bysPage + rec_pos;

    for (int i = 0; i < _vctRecord.size(); i++) {
      WriteShort(off_pos, rec_pos);
      BranchRecord *rr = (BranchRecord *)_vctRecord[i];
      uint16_t sz = rr->SaveData(bys_rec);
      rr->UpdateBysValue(bys_rec);
      bys_rec += sz;
      rec_pos += sz;
      off_pos += UI16_LEN;
    }

    CachePool::ReleasePage(tmp);
    WriteShort(TOTAL_DATA_LENGTH_OFFSET, _committedDataLength);
    WriteShort(NUM_RECORD_OFFSET, _recordNum);
    _bRecordUpdated = false;
  }

  WriteInt(PARENT_PAGE_POINTER_OFFSET, _parentPageId);

  boost::crc_32_type crc32;
  crc32.process_bytes(_bysPage, CRC32_INDEX_OFFSET);
  WriteInt(CRC32_INDEX_OFFSET, crc32.checksum());
  _bDirty = false;

  if (bUnlock) {
    _spinLock.unlock();
  }

  _bNeedDisk = true;
  return true;
}

BranchRecord *BranchPage::DeleteRecord(uint16_t index) {
  assert(_pageStatus.load(memory_order_relaxed) == PageStatus::VALID ||
         _pageStatus.load(memory_order_relaxed) == PageStatus::WRITING);
  assert(index >= 0 && index < _recordNum);

  BranchRecord *brDel = (BranchRecord *)_vctRecord[index];
  _committedDataLength -= brDel->GetTotalLength() + UI16_LEN;
  _recordNum--;
  _vctRecord.erase(_vctRecord.begin() + index);
  _bDirty = true;
  _bRecordUpdated = true;
  return brDel;
}

void BranchPage::InsertRecord(BranchRecord *record, int32_t pos) {
  assert(_pageStatus.load(memory_order_relaxed) == PageStatus::VALID ||
         _pageStatus.load(memory_order_relaxed) == PageStatus::WRITING);
  assert(pos >= 0 && pos <= _recordNum);

  _committedDataLength += record->GetTotalLength() + UI16_LEN;
  _vctRecord.insert(_vctRecord.begin() + pos, record);
  _recordNum++;
  _bRecordUpdated = true;
  _bDirty = true;
}

bool BranchPage::AppendRecord(BranchRecord *rr) {
  if (_committedDataLength > MAX_DATA_LENGTH_BRANCH * LOAD_FACTOR / 100U ||
      _committedDataLength + rr->GetTotalLength() + UI16_LEN >
          MAX_DATA_LENGTH_BRANCH) {
    return false;
  }

  _committedDataLength += rr->GetTotalLength() + UI16_LEN;
  _vctRecord.push_back(rr);
  _recordNum++;
  _bRecordUpdated = true;
  _bDirty = true;

  return true;
}

int32_t BranchPage::SearchRecord(const RawRecord &rr) const {
  bool bUnique =
      (_indexTree->GetHeadPage()->GetIndexType() != IndexType::NON_UNIQUE);
  int32_t start = 0;
  int32_t end = _recordNum - 1;

  while (true) {
    if (start > end) {
      if (start >= _recordNum) {
        start = _recordNum - 1;
      }

      return start;
    }

    int middle = (start + end) / 2;
    int hr = bUnique ? GetVctRecord(middle)->CompareKey(rr)
                     : GetVctRecord(middle)->CompareTo(rr);
    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      return middle;
    }
  }
}

int32_t BranchPage::SearchKey(const RawKey &key) const {
  bool bUnique =
      (_indexTree->GetHeadPage()->GetIndexType() != IndexType::NON_UNIQUE);
  int32_t start = 0;
  int32_t end = _recordNum - 1;

  while (true) {
    if (start > end) {
      if (start >= _recordNum) {
        start = _recordNum - 1;
      }

      return start;
    }

    int32_t middle = (start + end) / 2;
    int hr = GetVctRecord(middle)->CompareKey(key);
    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (!bUnique && middle > start &&
          GetVctRecord(middle - 1)->CompareKey(key) == 0) {
        end = middle - 1;
      } else {
        return middle;
      }
    }
  }
}

BranchRecord &BranchPage::GetRecord(int32_t pos, bool bAutoLast) {
  assert(_recordNum > 0 && pos >= 0);
  assert(bAutoLast || pos < _recordNum);
  if (bAutoLast && pos >= _recordNum) {
    pos = _recordNum - 1;
  }

  return *GetVctRecord(pos);
}

void BranchPage::SetChild(int32_t pos, IndexPage *child) {
  assert(pos >= 0 && pos < _recordNum);
  assert(_vctRecord.size() == _recordNum);
  BranchRecord *br = (BranchRecord *)_vctRecord[pos];
  br->SetChildPage(child);
}

IndexPage *BranchPage::GetChild(int32_t pos) {
  assert(pos >= 0 && pos < _recordNum);
  assert(_vctRecord.size() == _recordNum);
  BranchRecord *br = (BranchRecord *)_vctRecord[pos];
  return br->GetChildPage();
}

bool BranchPage::SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap) {
  bool block = (_indexTree->GetSplitPageLevel() != UINT8_MAX);
  if (_indexTree->GetSplitPageLevel() < GetPageLevel()) {
    if (!_spinLock.try_lock()) {
      return false;
    }
  }

  BranchRecord *brParentOld = nullptr;
  int posInParent = 0;

  if (_parentPageId == PAGE_NULL_POINTER) {
    _parentPage = (BranchPage *)_indexTree
                      ->ApplyIndexPages(nullptr, GetPageLevel() + 1, 1)
                      .at(0);
    _parentPage->SetBeginPage(true);
    _parentPage->SetEndPage(true);
    _parentPageId = _parentPage->GetPageId();
  } else {
    assert(_parentPage != nullptr &&
           (_parentPage->GetPageStatus() == PageStatus::VALID ||
            _parentPage->GetPageStatus() == PageStatus::WRITING));

    if (_indexTree->GetSplitPageLevel() < _parentPage->GetPageLevel()) {
      _parentPage->Lock();
    }

    BranchRecord br(_indexTree->GetHeadPage()->GetIndexType(),
                    _vctRecord[_recordNum - 1], GetPageId());
    posInParent = ((BranchPage *)_parentPage)->SearchRecord(br);
    brParentOld = ((BranchPage *)_parentPage)->DeleteRecord(posInParent);
  }

  int limitLen = GetMaxDataLength() * LOAD_FACTOR / 100;
  int pos = 0;
  int len = 0;

  MVector<int> vctPos;
  MVector<int> vctLen;
  for (; pos < (int)_vctRecord.size(); pos++) {
    RawRecord *rr = _vctRecord[pos];

    len += rr->GetTotalLength() + sizeof(uint16_t);
    if (len > limitLen) {
      if (len > GetMaxDataLength()) {
        len -= rr->GetTotalLength() + sizeof(uint16_t);
        pos--;
      }

      vctPos.push_back(pos + 1);
      vctLen.push_back(len);
      len = 0;
    }
  }

  if (len > 0) {
    vctPos.push_back(pos);
    vctLen.push_back(len);
  }

  _committedDataLength = vctLen[0];
  _recordNum = vctPos[0];

  Byte level = GetPageLevel();
  MVector<IndexPage *> vctPage =
      _indexTree->ApplyIndexPages(_parentPage, level, vctPos.size() - 1);

  for (size_t i = 0; i < vctPage.size(); i++) {
    BranchPage *newPage = (BranchPage *)vctPage[i];
    newPage->_vctRecord.insert(newPage->_vctRecord.end(),
                               _vctRecord.begin() + vctPos[i],
                               _vctRecord.begin() + vctPos[i + 1]);
    newPage->SetDirty();
    newPage->SetRecordUpdated();
    newPage->_committedDataLength = vctLen[i + 1];
    newPage->_recordNum = (uint32_t)newPage->_vctRecord.size();
  }

  _vctRecord.erase(_vctRecord.begin() + vctPos[0], _vctRecord.end());

  if (IsEndPage()) {
    SetEndPage(false);
    vctPage[vctPage.size() - 1]->SetEndPage(true);
  }

  // Insert this page' key and id to parent page
  RawRecord *last = _vctRecord[_vctRecord.size() - 1];
  BranchRecord *rec = new BranchRecord(
      _indexTree->GetHeadPage()->GetIndexType(), last, GetPageId(), this);
  _parentPage->InsertRecord(rec, posInParent);
  posInParent++;

  // Insert new page' key and id to parent page
  for (int i = 0; i < (int)vctPage.size(); i++) {
    BranchPage *brPage = (BranchPage *)vctPage[i];
    last = (BranchRecord *)brPage->_vctRecord[brPage->_recordNum - 1];

    rec = nullptr;
    if (i == vctPage.size() - 1 && brParentOld != nullptr &&
        brParentOld->CompareTo(*last) > 0) {
      rec = new BranchRecord(_indexTree->GetHeadPage()->GetIndexType(),
                             brParentOld, brPage->GetPageId(), brPage);
    } else {
      rec = new BranchRecord(_indexTree->GetHeadPage()->GetIndexType(), last,
                             brPage->GetPageId(), brPage);
    }

    _parentPage->InsertRecord(rec, posInParent + i);

    for (RawRecord *rr : brPage->_vctRecord) {
      BranchRecord *br = (BranchRecord *)rr;
      IndexPage *childPage = br->GetChildPage();

      if (childPage == nullptr) {
        childPage = _indexTree->GetPage(br->GetChildPageId(),
                                        PageType::BRANCH_PAGE, brPage);
        br->SetChildPage(childPage);
      }

      childPage->SetParentPage(brPage);
      childPage->SetParentPageID(brPage->GetPageId());
      childPage->SetDirty();
      childPage->AddWriteQueue(pageMap);
    }

    brPage->SetRecordUpdated();
    brPage->SetDirty();
    brPage->SaveRecords();
    brPage->AddWriteQueue(pageMap);
  }

  if (GetPageLevel() == _indexTree->GetSplitPageLevel()) {
    int pos = _indexTree->CalcIndexRange(*last);
    IndexRange &range = _indexTree->GetVctRange().at(pos);
    size_t rpos = 0;
    for (; rpos < range._vctRangePage.size(); rpos++) {
      if (range._vctRangePage[rpos] == this) {
        break;
      }
    }

    assert(rpos < range._vctRangePage.size());
    rpos++;
    for (int ii = 0; ii < vctPage.size(); ii++) {
      range._vctRangePage.insert(range._vctRangePage.begin() + rpos + ii,
                                 (BranchPage *)vctPage[ii]);
    }
  }

  SetRecordUpdated();
  SaveRecords();
  SetDirty();
  AddWriteQueue(pageMap);
  _parentPage->SetRecordUpdated();
  _parentPage->SetDirty();
  _parentPage->AddWriteQueue(pageMap);

  if (_indexTree->GetSplitPageLevel() < GetPageLevel()) {
    _spinLock.unlock();
  }
  if (brParentOld != nullptr &&
      _indexTree->GetSplitPageLevel() < _parentPage->GetPageLevel()) {
    _parentPage->Unlock();
  }

  if (brParentOld != nullptr) {
    delete brParentOld;
    if (_parentPage->NeedForceSplit()) {
      _parentPage->SplitPage(pageMap);
    }
  } else {
    _indexTree->UpdateRootPage(_parentPage);
  }

  return true;
}

void BranchPage::ClearChild(IndexPage *child) {
  if (child->GetPageType() == PageType::BRANCH_PAGE) {
    BranchPage *bp = (BranchPage *)child;
    BranchRecord &br = bp->GetRecord(INT32_MAX, true);
    int32_t pos = SearchRecord(br);

    BranchRecord &brp = GetRecord(pos, true);
    assert(child == brp.GetChildPage());
    brp.SetChildPage(nullptr);
  } else {
    LeafPage *lp = (LeafPage *)child;
    LeafRecord &lr = lp->GetRecord(lp->GetRecordNumber() - 1);
    BranchRecord br(lr.GetIndexType(), &lr, lp->GetPageId());
    int32_t pos = SearchRecord(br);

    BranchRecord &brp = GetRecord(pos, true);
    assert(child == brp.GetChildPage());
    brp.SetChildPage(nullptr);
  }
}

LeafPage *BranchPage::GetLeftLeafChild() {
  BranchPage *bp = this;
  while (true) {
    BranchRecord &br = bp->GetRecord(0, false);
    IndexPage *child = br.GetChildPage();
    if (child == nullptr) {
      PageType type = (bp->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                               : PageType::BRANCH_PAGE);
      child = _indexTree->GetPage(br.GetChildPageId(), type, bp,
                                  type != PageType::LEAF_PAGE);
    }

    if (child->GetPageType() == PageType::LEAF_PAGE) {
      return (LeafPage *)child;
    } else {
      bp = (BranchPage *)child;
    }
  }
}

LeafPage *BranchPage::GetRightLeafChild() {
  BranchPage *bp = this;
  while (true) {
    BranchRecord &br = bp->GetRecord(bp->GetRecordNumber() - 1, false);
    IndexPage *child = br.GetChildPage();
    if (child == nullptr) {
      PageType type = (bp->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                               : PageType::BRANCH_PAGE);
      child = _indexTree->GetPage(br.GetChildPageId(), type, bp,
                                  type != PageType::LEAF_PAGE);
    }

    if (child->GetPageType() == PageType::LEAF_PAGE) {
      return (LeafPage *)child;
    } else {
      bp = (BranchPage *)child;
    }
  }
}

IndexPage *BranchPage::GetNextPage(IndexPage *currPage) {
  assert(!IsEndPage());
  auto iter = _vctRecord.rbegin();
  if (dynamic_cast<BranchRecord *>(*iter)->GetChildPage() == currPage) {
    BranchPage *bp = dynamic_cast<BranchPage *>(_parentPage->GetNextPage(this));
    IndexPage *ip = bp->GetChild(0);
    if (ip == nullptr) {
      ip = _indexTree->GetPage(bp->GetRecord(0, false).GetChildPageId(),
                               currPage->GetPageType(), bp,
                               currPage->GetPageLevel() != 0);
      bp->SetChild(0, ip);
    }

    return ip;
  }

  iter++;
  for (; iter != _vctRecord.rend(); iter++) {
    if (dynamic_cast<BranchRecord *>(*iter)->GetChildPage() == currPage) {
      iter--;

      BranchRecord *br = dynamic_cast<BranchRecord *>(*iter);
      IndexPage *ip = br->GetChildPage();
      if (ip == nullptr) {
        ip = _indexTree->GetPage(br->GetChildPageId(), currPage->GetPageType(),
                                 this, currPage->GetPageLevel() != 0);
        br->SetChildPage(ip);
      }

      return ip;
    }
  }

  abort();
  return nullptr;
}

void BranchPage::FillNextPage(IndexPage *currPage, bool bAll) {
  assert(GetPageLevel() == 1);
  LeafPage *lpCurr = dynamic_cast<LeafPage *>(currPage);
  auto iter = _vctRecord.rbegin();
  if (dynamic_cast<BranchRecord *>(*iter)->GetChildPage() == currPage) {
    LeafPage *lpNext = dynamic_cast<LeafPage *>(GetNextPage(currPage));
    lpCurr->SetNextPage(lpNext);
    return;
  }

  LeafPage *lpNext = dynamic_cast<LeafPage *>(
      dynamic_cast<BranchRecord *>(*iter)->GetChildPage());
  if (lpNext == nullptr) {
    lpNext == dynamic_cast<LeafPage *>(_indexTree->GetPage(
                  dynamic_cast<BranchRecord *>(*iter)->GetChildPageId(),
                  PageType::LEAF_PAGE, this));
    dynamic_cast<BranchRecord *>(*iter)->SetChildPage(lpNext);
  }

  iter++;
  for (; iter != _vctRecord.rend(); iter++) {
    lpCurr = dynamic_cast<LeafPage *>(
        dynamic_cast<BranchRecord *>(*iter)->GetChildPage());
    if (lpCurr == nullptr) {
      lpCurr = dynamic_cast<LeafPage *>(_indexTree->GetPage(
          dynamic_cast<BranchRecord *>(*iter)->GetChildPageId(),
          PageType::LEAF_PAGE, this));
      dynamic_cast<BranchRecord *>(*iter)->SetChildPage(lpCurr);
    }

    if (lpCurr->GetNextPage() == nullptr) {
      lpCurr->SetNextPage(lpNext);
    }

    assert(lpCurr->GetNextPage() == lpNext);
    if (!bAll && lpCurr == currPage) {
      break;
    }

    lpNext = lpCurr;
  }
}

} // namespace storage
