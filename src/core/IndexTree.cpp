#include "IndexTree.h"
#include "../pool/CachePagePool.h"
#include "../pool/FilePagePool.h"
#include "../utils/Log.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexPage.h"
#include "LeafPage.h"
#include <shared_mutex>

namespace storage {
IndexRange::~IndexRange() {
  assert(_queueAction.size() == 0);
  assert(_queueActionFromCollect.RoughSize() == 0);
  assert(_queueActionFromPrev.RoughSize() == 0);
  delete _borderRecord;
}

IndexPage *IndexRange::GetTopPage(IndexType type, RawRecord &rr) {
  assert(_vctRangePage.size() > 0);
  for (size_t i = 0; i < _vctRangePage.size() - 1; i++) {
    BranchRecord &br = _vctRangePage[i]->GetRecord(INT32_MAX, true);
    if (type == IndexType::NON_UNIQUE) {
      if (br.CompareTo(rr) >= 0) {
        return _vctRangePage[i];
      }
    } else {
      if (br.CompareKey(rr) >= 0) {
        return _vctRangePage[i];
      }
    }
  }

  return _vctRangePage[_vctRangePage.size() - 1];
}

IndexPage *IndexRange::GetTopPage(RawKey &key) {
  assert(_vctRangePage.size() > 0);
  for (size_t i = 0; i < _vctRangePage.size() - 1; i++) {
    BranchRecord &br = _vctRangePage[i]->GetRecord(INT32_MAX, true);
    if (br.CompareKey(key) >= 0) {
      return _vctRangePage[i];
    }
  }

  return _vctRangePage[_vctRangePage.size() - 1];
}

IndexTree::~IndexTree() {
  while (_pagesInMem.load(memory_order_acquire) > 0) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }

  _garbageOwner->SavePage(true);
  delete _garbageOwner;
  _garbageOwner = nullptr;

  while (!_headPage->SaveToBuffer()) {
    this_thread::yield();
  }

  FilePagePool::SyncWritePage(_headPage);
  delete _headPage;
  _headPage = nullptr;

  _fileHandle->Close();
  delete _fileHandle;
  LOG_DEBUG << "Close index tree " << _indexName;
}

bool IndexTree::CreateIndexTree(const MString &tableName,
                                const MString &indexName,
                                const MString &fileName,
                                VectorDataValue &vctKey,
                                VectorDataValue &vctVal, uint32_t indexId,
                                IndexType iType) {
  assert(_headPage == nullptr);
  _tableName = tableName;
  _indexName = indexName;
  _fileName = fileName;
  for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
    if (*iter == '\\')
      *iter = '/';
  }

  _fileId = indexId;
  _fileHandle = FileHandle::OpenFile(_fileName);
  if (_fileHandle == nullptr) {
    return false;
  }

  _headPage = new HeadPage(this);
  _headPage->InitHeadPage(iType, vctVal);

  _garbageOwner = new GarbageOwner(this);
  _rootPage = ApplyIndexPages(nullptr, 0, 1).at(0);
  _rootPage->SetBeginPage(true);
  _rootPage->SetEndPage(true);
  _rootPage->SetDirty();

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);

  if (_headPage->GetIndexType() == IndexType::PRIMARY) {
    _valVarLen = _headPage->GetValueVariableFieldCount() * UI32_LEN;
    _valOffset = _valVarLen + (uint16_t)((_vctValue.size() + 7) >> 3);
  } else {
    _valVarLen = 0;
    _valOffset = 0;
  }

  _indexType = iType;
  LOG_DEBUG << "Create index tree " << indexName;
  return true;
}

bool IndexTree::LoadIndexTree(const MString &tableName,
                              const MString &indexName, const MString &fileName,
                              VectorDataValue &vctKey, VectorDataValue &vctVal,
                              uint32_t indexId) {
  assert(_headPage == nullptr);
  _tableName = tableName;
  _indexName = indexName;
  _fileName = fileName;
  _fileId = indexId;
  for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
    if (*iter == '\\')
      *iter = '/';
  }

  _fileHandle = FileHandle::OpenFile(_fileName);
  _headPage = new HeadPage(this);
  FilePagePool::SyncReadPage(_headPage);
  _headPage->InitParameters();
  FileVersion &&fv = _headPage->ReadFileVersion();
  if (!(fv == CURRENT_FILE_VERSION)) {
    _threadErrorMsg.reset(
        new ErrorMsg(TB_ERROR_INDEX_VERSION, {_fileName.c_str()}));
    return false;
  }

  _indexType = _headPage->GetIndexType();
  _garbageOwner = new GarbageOwner(this);
  uint32_t rootId = _headPage->GetRootPageID();
  if (rootId == 0) {
    _rootPage = new LeafPage(this, rootId);
  } else {
    _rootPage = new BranchPage(this, rootId);
  }

  FilePagePool::SyncReadPage(_rootPage);
  _rootPage->AfterRead();
  _rootPage->SetPageStatus(PageStatus::VALID);
  IncPages();
  _rootPage->SetReferred(true);
  CachePagePool::AddPage(_rootPage);

#ifdef _DEBUG
  uint16_t count = 0;
  for (IDataValue *dv : vctVal) {
    if (!dv->IsFixLength())
      count++;
  }
  assert(count == _headPage->GetValueVariableFieldCount());
#endif

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);

  if (_headPage->GetIndexType() == IndexType::PRIMARY) {
    _valVarLen = _headPage->GetValueVariableFieldCount() * UI32_LEN;
    _valOffset = _valVarLen + (uint16_t)((_vctValue.size() + 7) >> 3);
  } else {
    _valVarLen = 0;
    _valOffset = 0;
  }

  LOG_DEBUG << "Open index tree " << indexName;
  return true;
}

void IndexTree ::Close() {
  if (_rootPage != nullptr) {
    ReleaseIndexPage(_rootPage);
    _rootPage = nullptr;
  }

  _bClosed.store(true, memory_order_release);
}

void IndexTree::CloneKeys(VectorDataValue &vct) {
  assert(vct.size() == 0);
  vct.reserve(_vctKey.size());

  for (IDataValue *dv : _vctKey) {
    vct.push_back(dv->Clone(false));
  }
}

void IndexTree::CloneValues(VectorDataValue &vct) {
  assert(vct.size() == 0);
  vct.reserve(_vctValue.size());

  for (IDataValue *dv : _vctValue) {
    vct.push_back(dv->Clone(false));
  }
}

MVector<IndexPage *> IndexTree::ApplyIndexPages(BranchPage *parentPage,
                                                Byte pageLevel, uint32_t pnum) {
  bool block = (GetSplitPageLevel() != UINT8_MAX);
  MVector<PageID> vctId = _garbageOwner->ApplyIndexPages(pnum, block);

  if (vctId.size() < (size_t)pnum) {
    uint32_t len = pnum - vctId.size();
    PageID pid = _headPage->GetAndIncTotalPageCount(len, block);
    for (uint32_t i = 0; i < len; i++) {
      vctId.push_back(pid + i);
    }
  }

  MVector<IndexPage *> vctPage;
  PageID pid =
      (parentPage == nullptr ? PAGE_NULL_POINTER : parentPage->GetPageId());
  for (PageID id : vctId) {
    IndexPage *page = nullptr;
    if (0 != pageLevel) {
      page = new BranchPage(this, id, pageLevel, pid);
    } else {
      page = new LeafPage(this, id, pid);
    }

    page->SetParentPage(parentPage);
    page->SetPageStatus(PageStatus::VALID);
    page->GetBysPage()[IndexPage::PAGE_BEGIN_END_OFFSET] = 0;
    page->SetReferred(true);
    vctPage.push_back(page);

    LOG_DEBUG << "Allocate new CachePage, pageLevel=" << (int)pageLevel
              << "  pageId=" << id;
  }

  CachePagePool::AddPages(vctPage);
  IncPages(vctId.size());
  return vctPage;
}

OverflowPage *IndexTree::ApplyOvfPage(uint16_t num) {
  bool block = (GetSplitPageLevel() != UINT8_MAX);
  PageID pid = _garbageOwner->ApplyOvfPage(num, block);
  if (pid == PAGE_NULL_POINTER) {
    pid = _headPage->GetAndIncTotalPageCount(num, block);
  }

  return OverflowPage::GetPage(this, pid, num, true);
}

IndexPage *IndexTree::GetPage(PageID pageId, PageType type,
                              BranchPage *parentPage, bool bSyncRead) {
  assert(pageId < _headPage->GetTotalPageCount());
  IndexPage *page = (IndexPage *)CachePagePool::GetPage(this, pageId, type);

  if (parentPage != nullptr) {
    page->SetParentPage(parentPage);
  }

  if (page->GetPageStatus() == PageStatus::EMPTY) {
    if (bSyncRead) {
      bool b = FilePagePool::SyncReadPage(page);
      assert(b);
      page->AfterRead();
      page->SetPageStatus(PageStatus::VALID);
    } else {
      FilePagePool::AddReadPage(ThreadPool::GetThreadId(), page);
    }
  }

  return page;
}

/**
 * @brief
 */
bool IndexTree::SearchPage(const RawKey &key, IndexPage *&page) {
  assert(page != nullptr);
  while (true) {
    PageStatus status = page->GetPageStatus();
    if (status == PageStatus::READED) {
      page->SetPageStatus(PageStatus::VALID, true);
    } else if (status != PageStatus::VALID && status != PageStatus::WRITING) {
      return false;
    }

    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return true;
    }

    BranchPage *bPage = (BranchPage *)page;
    uint32_t pos = bPage->SearchKey(key);
    BranchRecord &br = bPage->GetRecord(pos, true);
    IndexPage *childPage = br.GetChildPage();
    if (childPage == nullptr) {
      uint32_t pageId = br.GetChildPageId();
      childPage = GetPage(pageId,
                          page->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                                    : PageType::BRANCH_PAGE,
                          bPage);
      br.SetChildPage(childPage);
    }

    page = childPage;
  }
}

bool IndexTree::SearchPage(const LeafRecord &lr, IndexPage *&page) {
  assert(page != nullptr);

  while (true) {
    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return true;
    }

    BranchPage *bPage = (BranchPage *)page;
    uint32_t pos = bPage->SearchRecord(lr);
    BranchRecord &br = bPage->GetRecord(pos, true);
    IndexPage *childPage = br.GetChildPage();
    if (childPage == nullptr) {
      uint32_t pageId = br.GetChildPageId();
      IndexPage *childPage =
          GetPage(pageId, page->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                                    : PageType::BRANCH_PAGE);
      br.SetChildPage(childPage);
      childPage->SetParentPage(bPage);
      if (childPage->GetPageStatus() != PageStatus::VALID) {
        return false;
      }
    }

    page = childPage;
  }
}

void IndexTree::SettleUpdatedPages(MTreeMap<uint64_t, CachePage *> &pageMap) {
  for (auto iter = pageMap.begin(); iter != pageMap.end(); iter++) {
    if (iter->second->GetPageType() == PageType::LEAF_PAGE ||
        iter->second->GetPageType() == PageType::BRANCH_PAGE) {
      IndexPage *page = (IndexPage *)iter->second;
      if (page->IsOverlength()) {
        page->SplitPage(pageMap);
      }
    }
  }

  for (auto iter = pageMap.begin(); iter != pageMap.end();) {
    bool block = false;
    bool move = true;
    switch (iter->second->GetPageType()) {
    case PageType::BRANCH_PAGE: {
      BranchPage *page = (BranchPage *)iter->second;
      assert(!page->IsOverlength());
      if (GetSplitPageLevel() < page->GetPageLevel()) {
        block = true;
        page->Lock();
      }

      if (page->IsDirty()) {
        bool b = page->SaveRecords();
        assert(b);
      }

      break;
    }
    case PageType::LEAF_PAGE: {
      LeafPage *page = (LeafPage *)iter->second;
      assert(!page->IsOverlength());
      if (page->IsDirty()) {
        bool b = page->SaveRecords(pageMap);
        if (!b) {
          move = false;
        }
      }
      break;
    }
    case PageType::HEAD_PAGE: {
      HeadPage *page = (HeadPage *)iter->second;
      page->SaveToBuffer();
      move = false;
      break;
    }
    default:
      break;
    }

    FilePagePool::AddWritePage(ThreadPool::GetThreadId(), iter->second, false);
    if (block) {
      iter->second->Unlock();
    }

    if (move) {
      iter->second->ClearWriteQueue();
      iter = pageMap.erase(iter);
    } else {
      iter++;
    }
  }

  FilePagePool::SubmitWritePage(ThreadPool::GetThreadId());
}

void IndexTree::ReleaseIndexPage(IndexPage *idxPage) {
  if (idxPage->GetParentPage() != nullptr) {
    BranchPage *parentPage = idxPage->GetParentPage();
    parentPage->ClearChild(idxPage);
  }

  MList<IndexPage *> lst;
  lst.push_back(idxPage);

  while (lst.size() > 0) {
    IndexPage *page = lst.front();
    lst.pop_front();

    while (page->GetPageStatus() != PageStatus::VALID) {
      this_thread::yield();
    }

    assert(!page->IsDirty());

    if (page->GetPageType() == PageType::BRANCH_PAGE) {
      BranchPage *bp = (BranchPage *)page;
      for (uint32_t i = 0; i < bp->GetRecordNumber(); i++) {
        BranchRecord *br = bp->GetVctRecord(i);
        if (br->GetChildPage() != nullptr) {
          lst.push_back(br->GetChildPage());
          br->SetChildPage(nullptr);
        }
      }
    } else {
      LeafPage *lp = (LeafPage *)page;
      LeafPage *pnext = lp->GetNextPage();
      if (pnext != nullptr) {
        if (pnext->GetParentPage() == nullptr) {
          lst.push_back(pnext);
        }

        lp->SetNextPage(nullptr);
      }

      LeafPage *pprev = lp->GetPrevPage();
      if (pprev != nullptr) {
        if (pprev->GetParentPage() != nullptr) {
          lst.push_back(pprev);
        }

        lp->SetPrevPage(nullptr);
      }
    }

    page->SetParentPage(nullptr);
    page->SetReferred(false);
  }
}

int IndexTree::CalcIndexRange(const RawRecord &rr) {
  assert(_vctRange.size() > 0);

  for (size_t i = 0; i < _vctRange.size() - 1; i++) {
    IndexRange &range = _vctRange[i];
    if (_indexType == IndexType::NON_UNIQUE) {
      if (range.GetLastRecord()->CompareTo(rr) >= 0) {
        return i;
      }
    } else {
      if (range.GetLastRecord()->CompareKey(rr) >= 0) {
        return i;
      }
    }
  }

  return _vctRange.size() - 1;
}

int IndexTree::CalcIndexRange(const RawKey &key) {
  assert(GetHeadPage()->GetIndexType() == IndexType::PRIMARY);
  assert(_vctRange.size() > 0);

  for (size_t i = 0; i < _vctRange.size() - 1; i++) {
    IndexRange &range = _vctRange[i];
    if (range.GetLastRecord()->CompareKey(key) >= 0) {
      return i;
    }
  }

  return _vctRange.size() - 1;
}

int IndexTree::CalcIndexRange(IndexPage *page) {
  assert(_vctRange.size() > 1);
  for (size_t i = 0; i < _vctRange.size() - 1; i++) {
    IndexRange &range = _vctRange[i];
    RawRecord *rr = page->_vctRecord[0];
    if (_indexType == IndexType::NON_UNIQUE) {
      if (range.GetLastRecord()->CompareTo(*rr) >= 0) {
        return i;
      }
    } else {
      if (range.GetLastRecord()->CompareKey(*rr) >= 0) {
        return i;
      }
    }
  }

  return _vctRange.size() - 1;
}

LeafRecord IndexTree::MakeMaxLeafRecord() {
  VectorDataValue vctKey, vctVal;
  CloneKeys(vctKey);
  CloneValues(vctVal);
  for (IDataValue *dv : vctKey) {
    dv->SetMaxValue();
  }

  for (IDataValue *dv : vctVal) {
    dv->SetMaxValue();
  }

  if (_indexType == IndexType::PRIMARY) {
    return LeafRecord(this, vctKey, vctVal, 1);
  } else {
    RawKey key(vctVal);
    return LeafRecord(this, vctKey, key.GetBysVal(), key.GetLength(),
                      ActionType::INSERT, 1);
  }
}

LeafRecord IndexTree::MakeMinLeafRecord() {
  VectorDataValue vctKey, vctVal;
  CloneKeys(vctKey);
  CloneValues(vctVal);
  for (IDataValue *dv : vctKey) {
    dv->SetMinValue();
  }

  for (IDataValue *dv : vctVal) {
    dv->SetMinValue();
  }

  if (_indexType == IndexType::PRIMARY) {
    return LeafRecord(this, vctKey, vctVal, 1);
  } else {
    RawKey key(vctVal);
    return LeafRecord(this, vctKey, key.GetBysVal(), key.GetLength(),
                      ActionType::INSERT, 1);
  }
}

void IndexTree::UpdateRecordNumber(int iRange, int64_t recNum) {
  if (_vctRange.size() > 1) {
    assert(iRange >= 0 && iRange < _vctRange.size());
    _vctRange[iRange]._recordNumber += recNum;
  } else {
    _headPage->GetAndIncTotalRecordCount(recNum, false);
  }
}

VersionStamp IndexTree::ApplyStamp(int iRange) {
  if (_vctRange.size() > 1) {
    assert(iRange >= 0 && iRange < _vctRange.size());
    IndexRange &range = _vctRange[iRange];
    if (range._recordStampStart >= range._recordStampEnd) {
      range._recordStampStart = _headPage->GetAndIncRecordStamp(STAMP_BATCH);
      range._recordStampEnd = range._recordStampStart + STAMP_BATCH;
    }

    VersionStamp tmp = range._recordStampStart;
    range._recordStampStart++;
    return tmp;
  } else {
    return _headPage->GetAndIncRecordStamp();
  }
}
} // namespace storage
