#include "IndexPage.h"
#include "../binlog/LogRecord.h"
#include "../binlog/LogServer.h"
#include "../pool/PageDividePool.h"
#include "../pool/StoragePool.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
const uint16_t IndexPage::LOAD_FACTOR = 90;
const uint32_t IndexPage::LOAD_THRESHOLD = CachePage::CACHE_PAGE_SIZE * 3;
const uint16_t IndexPage::PAGE_LEVEL_OFFSET = 0;
const uint16_t IndexPage::PAGE_BEGIN_END_OFFSET = 1;
const uint16_t IndexPage::PAGE_TRAN_COUNT = 2;
const uint16_t IndexPage::NUM_RECORD_OFFSET = 4;
const uint16_t IndexPage::TOTAL_DATA_LENGTH_OFFSET = 6;
const uint16_t IndexPage::PARENT_PAGE_POINTER_OFFSET = 8;

IndexPage::IndexPage(IndexTree *indexTree, uint32_t pageId, PageType type)
    : CachePage(indexTree, pageId, type) {}

IndexPage::IndexPage(IndexTree *indexTree, uint32_t pageId, uint8_t pageLevel,
                     uint32_t parentPageId, PageType type)
    : CachePage(indexTree, pageId, type) {
  _bysPage[PAGE_LEVEL_OFFSET] = (Byte)pageLevel;
  _parentPageId = parentPageId;
}

IndexPage::~IndexPage() {}

void IndexPage::Init() {
  _tranCount = 0;
  _recordNum = ReadShort(NUM_RECORD_OFFSET);
  _totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
  _parentPageId = ReadInt(PARENT_PAGE_POINTER_OFFSET);
}

bool IndexPage::PageDivide() {
  BranchRecord *brParentOld = nullptr;
  BranchPage *parentPage = nullptr;
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
    parentPage = (BranchPage *)_indexTree->GetPage(_parentPageId, false);
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

  if (startPos < _vctRecord.size()) {
    newPage = _indexTree->AllocateNewPage(parentPage->GetPageId(), level);
    newPage->SetDirty(true);
    newPage->WriteLock();
    newPage->_vctRecord.insert(newPage->_vctRecord.end(),
                               _vctRecord.begin() + startPos, _vctRecord.end());

    newPage->_totalDataLength = len;
    newPage->_recordNum = (int32_t)newPage->_vctRecord.size();
    newPage->_tranCount = tranCount;
    vctPage.push_back(newPage);
  }

  _vctRecord.erase(_vctRecord.begin() + pos, _vctRecord.end());

  // Insert this page' key and id to parent page
  RawRecord *last = _vctRecord[_vctRecord.size() - 1];
  BranchRecord *rec = new BranchRecord(_indexTree, last, GetPageId());
  parentPage->InsertRecord(rec, posInParent);
  posInParent++;

  if (_absoBuf == nullptr && refCount > 0) {
    _absoBuf = new AbsoleteBuffer(_bysPage, refCount);
  }
  // Insert new page' key and id to parent page
  for (int i = 0; i < vctPage.size(); i++) {
    IndexPage *indexPage = vctPage[i];
    last = indexPage->_vctRecord[indexPage->_recordNum - 1];

    rec = nullptr;
    if (i == vctPage.size() - 1 && brParentOld != nullptr &&
        brParentOld->CompareTo(*last) > 0) {
      rec = new BranchRecord(_indexTree, brParentOld, indexPage->GetPageId());
    } else {
      rec = new BranchRecord(_indexTree, last, indexPage->GetPageId());
    }

    parentPage->InsertRecord(rec, posInParent + i);
    indexPage->_absoBuf = _absoBuf;

    for (RawRecord *rr : indexPage->_vctRecord) {
      rr->SetParentPage(indexPage);
    }
  }

  // Add bin log record for page divid
  MVector<BranchRecord *> vctLog;
  for (int i = posInParent - 1; i < posInParent + vctPage.size(); i++) {
    vctLog.push_back((BranchRecord *)parentPage->_vctRecord[i]);
  }
  LogPageDivid *ld =
      new LogPageDivid(0, nullptr, 0, parentPage->GetPageId(), vctLog, this);
  LogServer::PushRecord(ld);

  parentPage->WriteUnlock();

  if (level == 0) {
    // if is leaf page, set left and right page
    uint32_t lastPointer = ((LeafPage *)this)->GetNextPageId();

    ((LeafPage *)this)->SetNextPageId(vctPage[0]->GetPageId());
    uint32_t prevPointer = GetPageId();

    for (int i = 0; i < vctPage.size() - 1; i++) {
      ((LeafPage *)vctPage[i])->SetPrevPageId(prevPointer);
      ((LeafPage *)vctPage[i])->SetNextPageId(vctPage[i + 1]->GetPageId());
      prevPointer = vctPage[i]->GetPageId();
    }

    ((LeafPage *)vctPage[vctPage.size() - 1])->SetPrevPageId(prevPointer);
    ((LeafPage *)vctPage[vctPage.size() - 1])->SetNextPageId(lastPointer);

    if (lastPointer == PAGE_NULL_POINTER) {
      _indexTree->GetHeadPage()->WriteEndLeafPagePointer(
          ((LeafPage *)vctPage[vctPage.size() - 1])->GetPageId());
    } else {
      LeafPage *lastPage = (LeafPage *)_indexTree->GetPage(lastPointer, true);
      lastPage->SetPrevPageId(
          ((LeafPage *)vctPage[vctPage.size() - 1])->GetPageId());
      lastPage->SetDirty(true);
      PageDividePool::AddCachePage(lastPage, false);
    }
  }

  if (IsEndPage()) {
    SetEndPage(false);
    vctPage[vctPage.size() - 1]->SetEndPage(true);
  }

  // Save new page'
  for (int i = 0; i < vctPage.size(); i++) {
    IndexPage *indexPage = vctPage[i];
    indexPage->_bRecordUpdate = true;
    PageDividePool::AddCachePage(indexPage, false);
    indexPage->WriteUnlock();
  }

  if (brParentOld != nullptr) {
    delete brParentOld;
  }

  _bRecordUpdate = true;
  // SaveRecords();
  PageDividePool::AddCachePage(this, false);
  PageDividePool::AddCachePage(parentPage, false);
  StoragePool::WriteCachePage(_indexTree->GetHeadPage(), false);

  return true;
}
} // namespace storage
