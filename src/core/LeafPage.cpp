#include "LeafPage.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "HeadPage.h"
#include "IndexTree.h"
#include "LeafRecord.h"

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
  _committedDataLength = _tempDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadInt(PARENT_PAGE_POINTER_OFFSET);

  _prevPageId = ReadInt(PREV_PAGE_POINTER_OFFSET);
  _nextPageId = ReadInt(NEXT_PAGE_POINTER_OFFSET);

  if (_parentPage != nullptr && _parentPageId != _parentPage->GetPageId())
      [[unlikely]] {
    _parentPageId = _parentPage->GetPageId();
    _bDirty = true;
  }
  if (_prevPage != nullptr && _prevPageId != _prevPage->GetPageId())
      [[unlikely]] {
    _prevPageId = _prevPage->GetPageId();
    _bDirty = true;
  }
  if (_nextPage != nullptr && _nextPageId != _nextPage->GetPageId())
      [[unlikely]] {
    _nextPageId = _nextPage->GetPageId();
    _bDirty = true;
  }

  LoadRecords();
}

void LeafPage::LoadRecords() {
  assert(!_bDirty && _vctRecord.size() == 0);

  uint16_t pos = DATA_BEGIN_OFFSET;
  for (uint16_t i = 0; i < _recordNum; i++) {
    LeafRecord *lr = new LeafRecord(_indexTree->GetHeadPage()->GetIndexType(),
                                    _bysPage + *((uint16_t *)(_bysPage + pos)));
    _vctRecord.push_back(lr);
    pos += sizeof(uint16_t);
  }
}

bool LeafPage::SaveRecords(MTreeMap<uint64_t, CachePage *> &pageMap,
                           bool block) {
  assert(_bDirty);
  if (GetPageStatus() != PageStatus::VALID ||
      _committedDataLength > MAX_DATA_LENGTH_LEAF)
    return false;

  MVector<LeafRecord *> vctLr;
  vctLr.reserve(_vctRecord.size());
  _committedDataLength = 0;
  _tempDataLength = 0;
  bool bClean = true;

  for (uint16_t i = 0; i < _vctRecord.size(); i++) {
    LeafRecord *lr = (LeafRecord *)_vctRecord[i];

    if (lr->GetLock() != nullptr && lr->ReleaseLockAble()) {
      _bRecordUpdated = true;
      ReleaseResult res = lr->ReleaseLock(GetIndexTree(), block);
      if (res == ReleaseResult::DELETED) {
        assert(lr->_overflowPage == nullptr);
        delete lr;
        _vctRecord.erase(_vctRecord.begin() + i);
        i--;
        continue;
      }
    }

    if (lr->IsStable()) {
      int n = lr->GetTotalLength() + UI16_LEN;
      _tempDataLength += n;
      _committedDataLength += n;
    } else {
      _tempDataLength += lr->GetTotalLength() + UI16_LEN;

      lr = lr->_recLock->_undoRec;
      while (lr != nullptr) {
        if (lr->IsStable()) {
          assert(lr->_recLock == nullptr || lr->_recLock->_undoRec == nullptr);
          _committedDataLength += lr->GetTotalLength() + UI16_LEN;
          break;
        }

        lr = lr->_recLock->_undoRec;
      }
      bClean = false;
    }

    if (lr != nullptr) {
      assert(lr->_recLock == nullptr || lr->_recLock->_undoRec == nullptr);
      vctLr.push_back(lr);
      if (lr->_overflowPage != nullptr) {
        lr->_overflowPage->AddWriteQueue(pageMap);
      }
    }
  }

  if (_committedDataLength > MAX_DATA_LENGTH_LEAF) {
    return false;
  }

  if (_bRecordUpdated) {
    Byte *tmp = _bysPage;
    _bysPage = CachePool::ApplyPage();

    _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
    _bysPage[PAGE_BEGIN_END_OFFSET] = tmp[PAGE_BEGIN_END_OFFSET];

    uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + vctLr.size() * UI16_LEN);
    uint16_t off_pos = DATA_BEGIN_OFFSET;
    for (uint16_t i = 0; i < vctLr.size(); i++) {
      LeafRecord *lr = vctLr[i];
      WriteShort(off_pos, pos);
      uint16_t sz = lr->SaveData(_bysPage + pos);
      lr->UpdateBysValue(_bysPage + pos);
      pos += sz;
      off_pos += UI16_LEN;
    }

    CachePool::ReleasePage(tmp);
    WriteShort(NUM_RECORD_OFFSET, (uint16_t)vctLr.size());
    WriteShort(TOTAL_DATA_LENGTH_OFFSET, _committedDataLength);

    if (bClean) {
      _bRecordUpdated = false;
    }
  }

  WriteInt(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
  WriteInt(PREV_PAGE_POINTER_OFFSET, _prevPageId);
  WriteInt(NEXT_PAGE_POINTER_OFFSET, _nextPageId);

  boost::crc_32_type crc32;
  crc32.process_bytes(_bysPage, CRC32_INDEX_OFFSET);
  WriteInt(CRC32_INDEX_OFFSET, crc32.checksum());
  _bDirty = bClean ? false : true;
  return bClean;
}

void LeafPage::InsertRecord(LeafRecord *lr, int32_t pos) {
  assert(lr->_recLock == nullptr ||
         lr->_recLock->_actType == ActionType::INSERT);
  assert(pos >= 0 && pos <= _recordNum);
  assert(_recordNum == 0 || _vctRecord.size() > 0);

  _tempDataLength += lr->GetTotalLength() + UI16_LEN;
  if (lr->GetLock() == nullptr) {
    _committedDataLength += lr->GetTotalLength() + UI16_LEN;
  }

  _vctRecord.insert(_vctRecord.begin() + pos, lr);
  _recordNum++;
  _bDirty = true;
  _bRecordUpdated = true;
}

void LeafPage::DeleteRecord(LeafRecord *lr, int32_t pos) {
  assert(lr->GetAction() == ActionType::DELETE);
  assert(pos >= 0 && pos <= _recordNum);
  assert(_recordNum == 0 || _vctRecord.size() > 0);

  LeafRecord *old = (LeafRecord *)_vctRecord[pos];
  assert(!old->_bDelete);

  RecordLock *lock = lr->_recLock;
  lock->_undoRec = old;
  _tempDataLength -= old->GetTotalLength() + UI16_LEN;
  _vctRecord[pos] = lr;
  _bDirty = true;
  _bRecordUpdated = true;
  lock->_recResult.store(RecordResult::IN_PAGE, memory_order_release);
}

bool LeafPage::AddRecord(LeafRecord *lr) {
  if (_committedDataLength + lr->GetTotalLength() + UI16_LEN >
      (uint32_t)MAX_DATA_LENGTH_LEAF) {
    return false;
  }

  if (_recordNum > 0 && _vctRecord.size() == 0) {
    LoadRecords();
  }

  assert(lr->GetLock() == nullptr);
  _committedDataLength += lr->GetTotalLength() + UI16_LEN;
  _vctRecord.push_back(lr);
  _recordNum++;
  _bDirty = true;
  _bRecordUpdated = true;
  return true;
}

void LeafPage::UpdateAction(LeafRecord *lr) {
  bool bFind;
  int pos = SearchRecord(*lr, bFind);

  if (bFind) {
    LeafRecord *old = (LeafRecord *)_vctRecord[pos];
    if (old->GetLock() != nullptr && old->ReleaseLockAble()) {
      int32_t commLen1, commLen2, tempLen1, tempLen2;
      old->GetLength(tempLen1, commLen1);
      old->ReleaseLock(_indexTree, _indexTree->IsMultiRange());
      old->GetLength(tempLen2, commLen2);

      _tempDataLength += tempLen2 - tempLen1;
      _committedDataLength += commLen2 - commLen1;
    }

    if (old->IsDelete()) {
      delete old;
      _vctRecord.erase(_vctRecord.begin() + pos);
      _recordNum--;
      bFind = false;
    }
  }

  RecordLock *lock = lr->GetLock();
  if (lr->GetAction() == ActionType::INSERT) {
    if (bFind) {
      lock->_errMsg = new ErrorMsg(
          STMT_DUPLICATE_ENTRY, {lr->GetKeyString(), _indexTree->GetTableName(),
                                 _indexTree->GetIndexName()});
      lock->_recResult.store(RecordResult::ERROR, memory_order_release);
    } else {
      _tempDataLength += lr->GetTotalLength() + UI16_LEN;
      _vctRecord.insert(_vctRecord.begin() + pos, lr);
      _recordNum++;
      _bDirty = true;
      _bRecordUpdated = true;
      lock->_recResult.store(RecordResult::IN_PAGE, memory_order_release);
    }
  } else if (lr->GetAction() == ActionType::DELETE) {
    assert(bFind);
    LeafRecord *old = (LeafRecord *)_vctRecord[pos];

    if (old->IsConflict(lock->TxID(), lock->_actType)) {
      lock->_errMsg = new ErrorMsg(STMT_LOCK_CONFLICT, {});
      lock->_recResult.store(RecordResult::ERROR, memory_order_release);
    } else {
      lock->_undoRec = old;
      _tempDataLength -= old->GetTotalLength() + UI16_LEN;
      _vctRecord[pos] = lr;
      _bDirty = true;
      _bRecordUpdated = true;
      lock->_recResult.store(RecordResult::IN_PAGE, memory_order_release);
    }
  } else {
    assert(lr->GetAction() == ActionType::UPSERT);
    if (bFind) {
      LeafRecord *old = (LeafRecord *)_vctRecord[pos];
      if (old->IsConflict(lock->TxID(), lock->_actType)) {
        lock->_errMsg = new ErrorMsg(STMT_LOCK_CONFLICT, {});
        lock->_recResult.store(RecordResult::ERROR, memory_order_release);
      } else {
        lock->_undoRec = old;
        _tempDataLength += lr->GetTotalLength() - old->GetTotalLength();
        _vctRecord[pos] = lr;
      }
    } else {
      _tempDataLength += lr->GetTotalLength() + UI16_LEN;
      _vctRecord.insert(_vctRecord.begin() + pos, lr);
      _recordNum++;
    }

    _bDirty = true;
    _bRecordUpdated = true;
    lock->_recResult.store(RecordResult::IN_PAGE, memory_order_release);
  }
}

LeafRecord &LeafPage::GetRecord(int32_t pos) {
  assert(pos >= 0 && pos < (int32_t)_recordNum);
  if (_vctRecord.size() == 0) {
    LoadRecords();
  }

  return *((LeafRecord *)_vctRecord[pos]);
}

int32_t LeafPage::SearchRecord(const LeafRecord &rr, bool &bFind, int32_t start,
                               int32_t end) {
  bool bUnique =
      (_indexTree->GetHeadPage()->GetIndexType() != IndexType::NON_UNIQUE);

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
    hr = bUnique ? GetRecord(middle).CompareKey(rr)
                 : GetRecord(middle).CompareTo(rr);

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
      (_indexTree->GetHeadPage()->GetIndexType() != IndexType::NON_UNIQUE);

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int32_t middle = (start + end) / 2;
    int hr = 0;
    hr = GetRecord(middle).CompareKey(key);

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (bUnique) {
        return middle;
      } else {
        if (middle > start && GetRecord(middle - 1).CompareKey(key) == 0) {
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
      (_indexTree->GetHeadPage()->GetIndexType() != IndexType::NON_UNIQUE);

  while (true) {
    if (start > end) {
      bFind = false;
      return start;
    }

    int32_t middle = (start + end) / 2;
    int hr = 0;
    hr = GetRecord(middle).CompareKey(rr);

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      if (bUnique) {
        return middle;
      } else {
        if (middle > start && GetRecord(middle - 1).CompareKey(rr) == 0) {
          end = middle - 1;
        } else {
          return middle;
        }
      }
    }
  }
}

void LeafPage::ClearRecords() {
  for (auto &lr : _vctRecord) {
    delete lr;
  }

  _vctRecord.clear();
}

bool LeafPage::SplitPage(MTreeMap<uint64_t, CachePage *> &pageMap,
                         Byte lockPageLevel) {
  if (_pageStatus.load(memory_order_relaxed) != PageStatus::VALID) {
    return false;
  }

  bool block = (lockPageLevel != UINT8_MAX);
  assert(lockPageLevel > GetPageLevel());

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
    posInParent = ((BranchPage *)_parentPage)->SearchRecord(br);
    if (posInParent > _parentPage->GetRecordNumber() - 1) {
      posInParent = _parentPage->GetRecordNumber() - 1;
    }

    brParentOld = ((BranchPage *)_parentPage)->DeleteRecord(posInParent);
  }

  int maxLen = GetMaxDataLength() * LOAD_FACTOR / 100;
  int pos = 0;
  int clen = 0;
  int tlen = 0;

  MVector<int> vctPos;
  MVector<int> vctCLen;
  MVector<int> vctTLen;

  for (; pos < (int)_vctRecord.size(); pos++) {
    LeafRecord *lr = (LeafRecord *)_vctRecord[pos];
    if (lr->ReleaseLockAble()) {
      ReleaseResult res = lr->ReleaseLock(GetIndexTree(), block);
      if (res == ReleaseResult::DELETED) {
        assert(lr->_overflowPage == nullptr);
        _vctRecord.erase(_vctRecord.begin() + pos);
        pos--;
        continue;
      }
    }

    int len = 0;
    if (!lr->IsStable()) {
      tlen += lr->GetTotalLength() + UI16_LEN;

      lr = lr->_recLock->_undoRec;
      while (lr != nullptr) {
        if (lr->IsStable()) {
          assert(lr->_recLock == nullptr || lr->_recLock->_undoRec == nullptr);
          len = lr->GetTotalLength() + UI16_LEN;
          clen += len;
          break;
        }

        lr = lr->_recLock->_undoRec;
      }
    } else {
      len = lr->GetTotalLength() + UI16_LEN;
      clen += len;
      tlen += len;
    }

    if (clen > maxLen || tlen > maxLen) {
      if (clen > GetMaxDataLength() || tlen > GetMaxDataLength()) {
        tlen -= lr->GetTotalLength() + sizeof(uint16_t);
        clen -= len;
        pos--;
      }

      vctPos.push_back(pos + 1);
      vctCLen.push_back(clen);
      vctTLen.push_back(tlen);
      clen = 0;
      tlen = 0;
    }
  }

  if (tlen > 0) {
    vctPos.push_back(pos);
    vctCLen.push_back(clen);
    vctTLen.push_back(tlen);
  }

  _committedDataLength = vctCLen[0];
  _tempDataLength = vctTLen[0];
  _recordNum = vctPos[0];

  MVector<IndexPage *> vctPage =
      _indexTree->ApplyIndexPages(_parentPage, 0, vctPos.size() - 1, block);

  for (size_t i = 0; i < vctPage.size(); i++) {
    LeafPage *newPage = (LeafPage *)vctPage[i];
    newPage->_vctRecord.insert(newPage->_vctRecord.end(),
                               _vctRecord.begin() + vctPos[i],
                               _vctRecord.begin() + vctPos[i + 1]);

    newPage->SetDirty();
    newPage->_committedDataLength = vctCLen[i + 1];
    newPage->_tempDataLength = vctTLen[i + 1];
    newPage->_recordNum = (uint32_t)newPage->_vctRecord.size();
  }

  _vctRecord.erase(_vctRecord.begin() + vctPos[0], _vctRecord.end());

  // Insert this page' key and id to parent page
  RawRecord *last = _vctRecord[_vctRecord.size() - 1];
  BranchRecord *rec = new BranchRecord(
      _indexTree->GetHeadPage()->GetIndexType(), last, GetPageId(), this);
  _parentPage->InsertRecord(rec, posInParent);
  posInParent++;

  // Insert new page' key and id to parent page
  for (int i = 0; i < (int)vctPage.size(); i++) {
    LeafPage *lfPage = (LeafPage *)vctPage[i];
    last = lfPage->_vctRecord[lfPage->_recordNum - 1];

    rec = nullptr;
    if (i == vctPage.size() - 1 && brParentOld != nullptr &&
        brParentOld->CompareTo(*last) > 0) {
      rec = new BranchRecord(_indexTree->GetHeadPage()->GetIndexType(),
                             brParentOld, lfPage->GetPageId(), lfPage);
    } else {
      rec = new BranchRecord(_indexTree->GetHeadPage()->GetIndexType(), last,
                             lfPage->GetPageId(), lfPage);
    }

    _parentPage->InsertRecord(rec, posInParent + i);
  }

  uint32_t lastId = ((LeafPage *)this)->GetNextPageId();
  LeafPage *lastPage = ((LeafPage *)this)->GetNextPage();

  SetNextPageId(vctPage[0]->GetPageId());
  SetNextPage((LeafPage *)vctPage[0]);
  uint32_t prevId = GetPageId();
  LeafPage *prevPage = this;

  for (int i = 0; i < vctPage.size() - 1; i++) {
    ((LeafPage *)vctPage[i])->SetPrevPageId(prevId);
    ((LeafPage *)vctPage[i])->SetPrevPage(prevPage);
    ((LeafPage *)vctPage[i])->SetNextPageId(vctPage[i + 1]->GetPageId());
    ((LeafPage *)vctPage[i])->SetNextPage((LeafPage *)vctPage[i + 1]);
    prevId = vctPage[i]->GetPageId();
    prevPage = (LeafPage *)vctPage[i];
  }

  ((LeafPage *)vctPage[vctPage.size() - 1])->SetPrevPageId(prevId);
  ((LeafPage *)vctPage[vctPage.size() - 1])->SetPrevPage(prevPage);
  ((LeafPage *)vctPage[vctPage.size() - 1])->SetNextPageId(lastId);
  if (lastId == PAGE_NULL_POINTER) {
    _indexTree->GetHeadPage()->SetEndLeafPageID(
        ((LeafPage *)vctPage[vctPage.size() - 1])->GetPageId());
    ((LeafPage *)vctPage[vctPage.size() - 1])->SetNextPage(nullptr);
    assert(IsEndPage());
    SetEndPage(false);
    ((LeafPage *)vctPage[vctPage.size() - 1])->SetEndPage(true);
  } else if (!IsRangEndPage()) {
    if (lastPage == nullptr) {
      lastPage = (LeafPage *)_indexTree->GetPage(lastId, PageType::LEAF_PAGE);
    }

    ((LeafPage *)vctPage[vctPage.size() - 1])->SetNextPage(lastPage);
    lastPage->SetPrevPage(((LeafPage *)vctPage[vctPage.size() - 1]));
    lastPage->SetPrevPageId((vctPage[vctPage.size() - 1])->GetPageId());
    lastPage->AddWriteQueue(pageMap);
  }

  if (IsRangEndPage()) {
    ((LeafPage *)vctPage[vctPage.size() - 1])->SetRangeEndPage(true);
    SetRangeEndPage(false);
    if (lastId != PAGE_NULL_POINTER) {
      size_t pos = _indexTree->CalcIndexRange(GetRecord(0)) + 1;
      PrevPageAction *act = new PrevPageAction(
          _indexTree, pos, lastId, (vctPage[vctPage.size() - 1])->GetPageId());
      _indexTree->AddActionFromPrev(pos, act);
    }
  }

  for (int i = 0; i < vctPage.size(); i++) {
    ((LeafPage *)vctPage[i])->SetRecordUpdated();
    ((LeafPage *)vctPage[i])->SaveRecords(pageMap, block);
    vctPage[i]->AddWriteQueue(pageMap);
  }

  SetRecordUpdated();
  SaveRecords(pageMap, block);
  AddWriteQueue(pageMap);
  _parentPage->SetRecordUpdated();
  _parentPage->AddWriteQueue(pageMap);

  if (lockPageLevel <= GetPageLevel()) {
    _spinLock.unlock();
  }

  if (brParentOld != nullptr && lockPageLevel <= _parentPage->GetPageLevel()) {
    _parentPage->Unlock();
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

LeafPage *LeafPage::GetPrevPage() {
  if (_prevPage != nullptr) {
    return _prevPage;
  }
  if (IsRangBeginPage() || _prevPageId == PAGE_NULL_POINTER) {
    return nullptr;
  }

  _prevPage = (LeafPage *)_indexTree->GetPage(_prevPageId, PageType::LEAF_PAGE);
  _prevPage->SetNextPage(this);
  return _prevPage;
}

LeafPage *LeafPage::GetNextPage() {
  if (_nextPage != nullptr) {
    return _nextPage;
  }
  if (IsRangEndPage() || _nextPageId == PAGE_NULL_POINTER) {
    return nullptr;
  }

  _nextPage = (LeafPage *)_indexTree->GetPage(_nextPageId, PageType::LEAF_PAGE);
  _nextPage->SetPrevPage(this);
  return _nextPage;
}

} // namespace storage
