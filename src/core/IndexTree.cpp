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
IndexTree::~IndexTree() {
  while (_pagesInMem.load(memory_order_acquire) > 0) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }

  _garbageOwner->SavePage(true);
  delete _garbageOwner;
  _garbageOwner = nullptr;

  if (_headPage->SaveToBuffer()) {
    FilePagePool::SyncWritePage(_headPage);
  }

  delete _headPage;
  _headPage = nullptr;

  _fileHandle->Close();
  delete _fileHandle;
  LOG_DEBUG << "Close index tree " << _indexName;
}

bool IndexTree::CreateIndexTree(const MString &indexName,
                                const MString &fileName,
                                VectorDataValue &vctKey,
                                VectorDataValue &vctVal, uint32_t indexId,
                                IndexType iType) {
  assert(_headPage == nullptr);
  _indexName = indexName;
  _fileName = fileName;
  for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
    if (*iter == '\\')
      *iter = '/';
  }

  _fileId = indexId;
  _fileHandle = FileHandle::OpenFile(_fileName);
  _headPage = new HeadPage(this);
  _headPage->InitHeadPage(iType, vctVal);

  _rootPage = ApplyIndexPages(nullptr, 0, 1, false).at(0);
  _rootPage->SetBeginPage(true);
  _rootPage->SetEndPage(true);
  _rootPage->SetDirty(true);

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);

  if (_headPage->GetIndexType() == IndexType::PRIMARY) {
    _valVarLen = _headPage->GetValueVariableFieldCount() * UI32_LEN;
    _valOffset = _valVarLen + (uint16_t)((_vctValue.size() + 7) >> 3);
  } else {
    _valVarLen = 0;
    _valOffset = 0;
  }

  _garbageOwner = new GarbageOwner(this);
  LOG_DEBUG << "Create index tree " << indexName;
  return true;
}

bool IndexTree::LoadIndexTree(const MString &indexName, const MString &fileName,
                              VectorDataValue &vctKey, VectorDataValue &vctVal,
                              uint32_t indexId) {
  assert(_headPage == nullptr);
  _indexName = indexName;
  _fileName = fileName;
  for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
    if (*iter == '\\')
      *iter = '/';
  }

  _fileHandle = FileHandle::OpenFile(_fileName);
  _headPage = new HeadPage(this);
  FilePagePool::SyncReadPage(_headPage);
  FileVersion &&fv = _headPage->ReadFileVersion();
  if (!(fv == CURRENT_FILE_VERSION)) {
    _threadErrorMsg.reset(
        new ErrorMsg(TB_ERROR_INDEX_VERSION, {_fileName.c_str()}));
    return false;
  }

  uint32_t rootId = _headPage->GetRootPageID();
  _rootPage = GetPage(rootId, rootId == 0 ? PageType::LEAF_PAGE
                                          : PageType::BRANCH_PAGE);

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

  _garbageOwner = new GarbageOwner(this);
  LOG_DEBUG << "Open index tree " << indexName;
  return true;
}

void IndexTree ::Close() {
  // unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
  _bClosed = true;
  if (_rootPage != nullptr) {
    _rootPage->SetReferred(false);
    _rootPage = nullptr;
  }
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
                                                Byte pageLevel, uint32_t pnum,
                                                bool block) {
  MVector<PageID> vctId = _garbageOwner->ApplyIndexPages(pnum, block);

  if (vctId.size() < (size_t)pnum) {
    uint32_t len = pnum - vctId.size();
    PageID pid = _headPage->GetAndIncTotalPageCount(len, block);
    for (uint32_t i = 0; i < len; i++) {
      vctId.push_back(pid + i);
    }
  }

  MVector<IndexPage *> vctPage;
  for (PageID id : vctId) {
    IndexPage *page = nullptr;
    if (0 != pageLevel) {
      page = new BranchPage(this, id, pageLevel, parentPage->GetPageId());
    } else {
      page = new LeafPage(this, id, parentPage->GetPageId());
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

OverflowPage *IndexTree::ApplyOvfPage(uint16_t num, bool block) {
  PageID pid = _garbageOwner->ApplyOvfPage(num, block);
  if (pid == PAGE_NULL_POINTER) {
    pid = _headPage->GetAndIncTotalPageCount(num, block);
  }

  return OverflowPage::GetPage(this, pid, num, true);
}

IndexPage *IndexTree::GetPage(PageID pageId, PageType type,
                              IndexPage *parentPage) {
  assert(pageId < _headPage->ReadTotalPageCount());
  IndexPage *page = (IndexPage *)CachePagePool::GetPage(_fileId, pageId);

  if (page == nullptr) {
    if (type == PageType::LEAF_PAGE) {
      page = new LeafPage(this, pageId);
    } else if (type == PageType::BRANCH_PAGE) {
      page = new BranchPage(this, pageId);
    } else {
      abort();
    }

    IncPages();
    FilePagePool::AddReadPage(ThreadPool::GetThreadId(), page);
  }

  if (parentPage != nullptr) {
    page->SetParentPage(parentPage);
  }

  return page;
}

/**
 * @brief
 */
bool IndexTree::SearchPage(const RawKey &key, IndexPage *&page) {
  assert(page != nullptr);
  while (true) {
    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return true;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchKey(key, bFind);
    BranchRecord &br = bPage->GetRecord(pos, true);
    IndexPage *childPage = br.GetChildPage();
    if (childPage == nullptr) {
      uint32_t pageId = br.GetChildPageId();
      childPage =
          GetPage(pageId, page->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                                    : PageType::BRANCH_PAGE);
      br.SetChildPage(childPage);
      childPage->SetParentPage(page);
      if (childPage->GetPageStatus() != PageStatus::VALID) {
        return false;
      }
    }

    page = childPage;
  }
}

bool IndexTree::SearchPage(const LeafRecord &lr, IndexPage *&page) {
  assert(page != nullptr);
  BranchRecord brs(GetHeadPage()->GetIndexType(), (RawRecord *)&lr, 0);

  while (true) {
    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return true;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchRecord(brs, bFind);
    BranchRecord &br = bPage->GetRecord(pos, true);
    IndexPage *childPage = br.GetChildPage();
    if (childPage == nullptr) {
      uint32_t pageId = br.GetChildPageId();
      IndexPage *childPage =
          GetPage(pageId, page->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                                    : PageType::BRANCH_PAGE);
      br.SetChildPage(childPage);
      childPage->SetParentPage(page);
      if (childPage->GetPageStatus() != PageStatus::VALID) {
        return false;
      }
    }

    page = childPage;
  }
}
} // namespace storage
