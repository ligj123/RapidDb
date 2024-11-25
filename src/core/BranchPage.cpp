#include "BranchPage.h"
#include "../pool/StoragePool.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
const uint16_t BranchPage::DATA_BEGIN_OFFSET = 12;
const uint16_t IndexPage::MAX_DATA_LENGTH_BRANCH =
    (uint16_t)(Configure::GetIndexPageSize() - BranchPage::DATA_BEGIN_OFFSET -
               sizeof(uint32_t));

void BranchPage::InitParameters() {
  _recordNum = ReadShort(NUM_RECORD_OFFSET);
  _committedDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadInt(PARENT_PAGE_POINTER_OFFSET);

  if (_parentPage != nullptr && _parentPage->GetPageId() != _parentPageId)
      [[unlikely]] {
    _parentPageId = _parentPage->GetPageId();
    _bDirty = true;
  }
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
  return true;
}

BranchRecord *BranchPage::DeleteRecord(uint16_t index) {
  assert(_pageStatus.load(memory_order_relaxed) == PageStatus::VALID ||
         _pageStatus.load(memory_order_relaxed) == PageStatus::WRITING);
  assert(index >= 0 && index < _recordNum);
  if (_vctRecord.size() == 0)
    LoadRecords();

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
  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  _committedDataLength += record->GetTotalLength() + UI16_LEN;
  _vctRecord.insert(_vctRecord.begin() + pos, record);
  _recordNum++;
  _bRecordUpdated = true;
  _bDirty = true;
}

bool BranchPage::AddRecord(BranchRecord *rr) {
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

bool BranchPage::KeyExist(const RawKey &key) const {
  if (_recordNum == 0) {
    return false;
  }

  bool bFind;
  SearchKey(key, bFind);
  return bFind;
}

int32_t BranchPage::SearchRecord(const RawRecord &rr, bool &bFind) const {
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
      (_indexTree->GetHeadPage()->GetIndexType() != IndexType::NON_UNIQUE);
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

int BranchPage::CompareTo(uint32_t recPos, const RawRecord &rr) const {
  assert(recPos < _recordNum);
  uint32_t startPos = ReadShort(DATA_BEGIN_OFFSET + recPos * UI16_LEN);
  uint32_t lenKey = ReadShort(startPos + UI16_LEN);

  if (rr.GetIndexType() != IndexType::NON_UNIQUE) {
    return BytesCompare(_bysPage + startPos + UI16_2_LEN,
                        ReadShort(startPos + UI16_LEN),
                        rr.GetBysValue() + UI16_2_LEN, rr.GetKeyLength());
  } else {
    return BytesCompare(_bysPage + startPos + UI16_2_LEN,
                        ReadShort(startPos) - UI16_2_LEN - PAGE_ID_LEN,
                        rr.GetBysValue() + UI16_2_LEN, rr.GetDataLength());
  }
}

int BranchPage::CompareTo(uint32_t recPos, const RawKey &key) const {
  assert(recPos < _recordNum);
  uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * UI16_LEN);

  return BytesCompare(_bysPage + start + UI16_2_LEN,
                      ReadShort(start + UI16_LEN), key.GetBysVal(),
                      key.GetLength());
}

BranchRecord &BranchPage::GetRecord(int32_t pos, bool bAutoLast) {
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

void BranchPage::SetChild(int32_t pos, IndexPage *child) {
  assert(pos > 0 && pos < _recordNum);
  assert(_vctRecord.size() == _recordNum);
  BranchRecord *br = (BranchRecord *)_vctRecord[pos];
  br->SetChildPage(child);
}

IndexPage *BranchPage::GetChild(int32_t pos) {
  assert(pos > 0 && pos < _recordNum);
  assert(_vctRecord.size() == _recordNum);
  BranchRecord *br = (BranchRecord *)_vctRecord[pos];
  return br->GetChildPage();
}

bool BranchPage::SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap,
                           Byte lockPageLevel) {
  if (_pageStatus.load(memory_order_relaxed) != PageStatus::VALID) {
    return false;
  }

  bool block = (lockPageLevel != UINT8_MAX);
  if (lockPageLevel <= GetPageLevel()) {
    if (_spinLock.try_lock()) {
      return false;
    }
  }

  BranchRecord *brParentOld = nullptr;
  int posInParent = 0;

  if (_parentPageId == PAGE_NULL_POINTER) {
    _parentPage = (BranchPage *)_indexTree
                      ->ApplyIndexPages(nullptr, GetPageLevel() + 1, 1, block)
                      .at(0);
    _parentPage->SetBeginPage(true);
    _parentPage->SetEndPage(true);
    _parentPageId = _parentPage->GetPageId();
  } else {
    assert(_parentPage != nullptr &&
           (_parentPage->GetPageStatus() == PageStatus::VALID ||
            _parentPage->GetPageStatus() == PageStatus::WRITING));

    if (lockPageLevel <= _parentPage->GetPageLevel()) {
      _parentPage->Lock();
    }

    BranchRecord br(_indexTree->GetHeadPage()->GetIndexType(),
                    _vctRecord[_recordNum - 1], GetPageId());
    bool bFind;
    posInParent = ((BranchPage *)_parentPage)->SearchRecord(br, bFind);
    if (!bFind) {
      posInParent = _parentPage->GetRecordNumber() - 1;
    }

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
      _indexTree->ApplyIndexPages(_parentPage, level, vctPos.size() - 1, block);

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

  SetRecordUpdated();
  SaveRecords();
  SetDirty();
  AddWriteQueue(pageMap);
  _parentPage->SetRecordUpdated();
  _parentPage->SetDirty();
  _parentPage->AddWriteQueue(pageMap);

  if (lockPageLevel <= GetPageLevel()) {
    if (brParentOld != nullptr &&
        lockPageLevel <= _parentPage->GetPageLevel()) {
      _parentPage->Unlock();
    }

    _spinLock.unlock();
  }

  if (brParentOld != nullptr) {
    delete brParentOld;
    if (_parentPage->NeedForceSplit()) {
      _parentPage->SplitPage(pageMap, lockPageLevel);
    }
  } else {
    _indexTree->UpdateRootPage(_parentPage, block);
  }

  return true;
}

void BranchPage::ClearChild(IndexPage *child) {
  if (child->GetPageType() == PageType::BRANCH_PAGE) {
    BranchPage *bp = (BranchPage *)child;
    BranchRecord &br = bp->GetRecord(INT32_MAX, true);
    bool bFind;
    int32_t pos = SearchRecord(br, bFind);
    if (!bFind) {
      pos = INT32_MAX;
    }

    BranchRecord &brp = GetRecord(pos, true);
    assert(child == brp.GetChildPage());
    brp.SetChildPage(nullptr);
  } else {
    LeafPage *lp = (LeafPage *)child;
    LeafRecord &lr = lp->GetRecord(lp->GetRecordNumber() - 1);
    BranchRecord br(lr.GetIndexType(), &lr, lp->GetPageId());
    bool bFind;
    int32_t pos = SearchRecord(br, bFind);
    if (!bFind) {
      pos = INT32_MAX;
    }

    BranchRecord &brp = GetRecord(pos, true);
    assert(child == brp.GetChildPage());
    brp.SetChildPage(nullptr);
  }
}
} // namespace storage
