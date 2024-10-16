﻿#include "IndexTree.h"
#include "../FilePagePool.h"
#include "../pool/PageBufferPool.h"
#include "../pool/PageDividePool.h"
#include "../pool/StoragePool.h"
#include "../utils/Log.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexPage.h"
#include "LeafPage.h"
#include <shared_mutex>

namespace storage {
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
  _headPage = new HeadPage(this);

  {
    memset(_headPage->GetBysPage(), 0, Configure::GetDiskClusterSize());
    _headPage->WriteFileVersion();
    _headPage->WriteRootPagePointer(0);
    _headPage->WriteTotalPageCount(0);
    _headPage->WriteTotalRecordCount(0);
    _headPage->WriteAutoIncrementKey(0);
    _headPage->WriteAutoIncrementKey2(0);
    _headPage->WriteAutoIncrementKey3(0);
    _headPage->WriteIndexType(iType);

    uint16_t count = 0;
    for (IDataValue *dv : vctKey) {
      if (!dv->IsFixLength())
        count++;
    }
    _headPage->WriteKeyVariableFieldCount(count);
    _headPage->SetPageStatus(PageStatus::VALID);

    count = 0;
    for (IDataValue *dv : vctVal) {
      if (!dv->IsFixLength())
        count++;
    }
    _headPage->WriteValueVariableFieldCount(count);
  }

  StoragePool::AddPage(_headPage, false);
  _rootPage = AllocateNewPage(PAGE_NULL_POINTER, 0);
  _rootPage->SetBeginPage(true);
  _rootPage->SetEndPage(true);
  ((LeafPage *)_rootPage)->SetPrevPageId(PAGE_NULL_POINTER);
  ((LeafPage *)_rootPage)->SetNextPageId(PAGE_NULL_POINTER);
  _rootPage->SetDirty(true);

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);

  if (_headPage->ReadIndexType() == IndexType::PRIMARY) {
    _valVarLen = _headPage->ReadValueVariableFieldCount() * UI32_LEN;
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

  _headPage = new HeadPage(this);
  FilePagePool::SyncReadPage(_headPage);
  FileVersion &&fv = _headPage->ReadFileVersion();
  if (!(fv == CURRENT_FILE_VERSION)) {
    _threadErrorMsg.reset(
        new ErrorMsg(TB_ERROR_INDEX_VERSION, {_fileName.c_str()}));
    return false;
  }

  uint32_t rootId = _headPage->ReadRootPagePointer();
  _rootPage = GetPage(
      rootId, rootId == 0 ? PageType::LEAF_PAGE : PageType::BRANCH_PAGE, true);

#ifdef _DEBUG
  uint16_t count = 0;
  for (IDataValue *dv : vctVal) {
    if (!dv->IsFixLength())
      count++;
  }
  assert(count == _headPage->ReadValueVariableFieldCount());
#endif

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);

  if (_headPage->ReadIndexType() == IndexType::PRIMARY) {
    _valVarLen = _headPage->ReadValueVariableFieldCount() * UI32_LEN;
    _valOffset = _valVarLen + (uint16_t)((_vctValue.size() + 7) >> 3);
  } else {
    _valVarLen = 0;
    _valOffset = 0;
  }

  _garbageOwner = new GarbageOwner(this);
  LOG_DEBUG << "Open index tree " << indexName;
  return true;
}

IndexTree::~IndexTree() {
  while (_pagesInMem.load(memory_order_acquire) > 0) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }

  _garbageOwner->SavePage();
  delete _garbageOwner;
  _garbageOwner = nullptr;

  if (_headPage->IsHeadChanged()) {
    _headPage->SaveToBuffer();
    FilePagePool::SyncWritePage(_headPage);
  }
  delete _headPage;
  _headPage = nullptr;

  if (_funcDestory != nullptr) {
    _funcDestory();
  }
  LOG_DEBUG << "Close index tree " << _indexName;
}

void IndexTree ::Close() {
  unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
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

vector<IndexPage *> IndexTree::ApplyIndexPages(PageID parentId, Byte pageLevel,
                                               uint32_t pnum, bool block) {
  vector<PageID> vctId = (_garbageOwner == nullptr)
                             ? {}
                             : _garbageOwner->ApplyIndexPages(pnum, block);
  if (vctId.size() < (size_t)pnum) {
    uint32_t len = pnum - vctId.size();
    PageID pid = _headPage->GetAndIncTotalPageCount(len, block);
    for (uint32_t i = 0; i < len; i++) {
      vctId.push_back(pid + i);
    }
  }

  vector<IndexPage *> vctPage;
  for (PageID id : vctId) {
    IndexPage *page = nullptr;
    if (0 != pageLevel) {
      page = new BranchPage(this, newPageId, pageLevel, parentId);
    } else {
      page = new LeafPage(this, newPageId, parentId);
    }

    page->SetPageStatus(PageStatus::VALID);
    page->GetBysPage()[IndexPage::PAGE_BEGIN_END_OFFSET] = 0;
    PageBufferPool::AddPage(page);
  }
  IncPages();

  LOG_DEBUG << "Allocate new CachePage, pageLevel=" << (int)pageLevel
            << "  pageId=" << newPageId;
  return page;
}

IndexPage *IndexTree::GetPage(PageID pageId, PageType type) {
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

    PageBufferPool::AddPage(page);
    IncPages();

    if (Configure::GetDiskType() == DiskType::SSD) {
      if (wait) {
        page->ReadPage(nullptr);

        if (page->GetPageStatus() != PageStatus::VALID) {
          // In following time will add code to fix page;
          abort();
        }
      } else {
        // For DiskType::SSD, multi threads load the pages at the same time is
        // more fast than single thread.
        ReadPageTask *task = new ReadPageTask(page);
        ThreadPool::InstMain().AddTask(task, false);
      }

      return page;
    } else {
      // For DiskType::HDD, it will rise a large read task and all read requests
      // will add the queue in large read task, then read one by one according
      // to the page id.
      // TO DO
      abort();
    }
  }

  if (wait)
    page->WaitRead();
  return page;
}

/**
 * @brief
 */
bool IndexTree::SearchRecursively(const RawKey &key, bool bEdit,
                                  IndexPage *&page, bool bWait) {
  if (page != nullptr) {
    if (bEdit && page->GetPageType() == PageType::LEAF_PAGE) {
      page->WriteLock();
    } else {
      page->ReadLock();
    }
  } else {
    while (true) {
      {
        std::shared_lock<SharedSpinMutex> lock(_rootSharedMutex);
        bool b = false;
        if (bEdit && _rootPage->GetPageType() == PageType::LEAF_PAGE) {
          b = _rootPage->WriteTryLock();
        } else {
          b = _rootPage->ReadTryLock();
        }

        if (b) {
          page = _rootPage;
          page->IncRef();

          if (page->GetPageType() == PageType::LEAF_PAGE)
            return true;
          else
            break;
        }
      }

      std::this_thread::yield();
    }
  }

  while (true) {
    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return true;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchKey(key, bFind);
    const BranchRecord &br = bPage->GetRecordByPos(pos, true);
    uint32_t pageId = br.GetChildPageId();

    IndexPage *childPage = (IndexPage *)GetPage(
        pageId,
        page->GetPageLevel() == 1 ? PageType::LEAF_PAGE : PageType::BRANCH_PAGE,
        bWait);
    assert(childPage != nullptr);

    if (childPage->GetPageStatus() != PageStatus::VALID && !bWait) {
      if (childPage->PushWaitTask(ThreadPool::_currTask)) {
        page->ReadUnlock();
        page->DecRef();
        page = childPage;
        return false;
      }
    }

    if (bEdit && childPage->GetPageType() == PageType::LEAF_PAGE) {
      childPage->WriteLock();
    } else {
      childPage->ReadLock();
    }

    page->ReadUnlock();
    page->DecRef();
    page = childPage;
  }
}

bool IndexTree::SearchRecursively(const LeafRecord &lr, bool bEdit,
                                  IndexPage *&page, bool bWait) {
  if (page != nullptr) {
    if (bEdit && page->GetPageType() == PageType::LEAF_PAGE) {
      page->WriteLock();
    } else {
      page->ReadLock();
    }
  } else {
    while (page == nullptr) {
      {
        std::shared_lock<SharedSpinMutex> lock(_rootSharedMutex);
        bool b = false;
        if (bEdit && _rootPage->GetPageType() == PageType::LEAF_PAGE) {
          b = _rootPage->WriteTryLock();
        } else {
          b = _rootPage->ReadTryLock();
        }

        if (b) {
          page = _rootPage;
          page->IncRef();
          if (page->GetPageType() == PageType::LEAF_PAGE)
            return true;
          else
            break;
        }
      }

      std::this_thread::yield();
    }
  }

  BranchRecord br(this, (RawRecord *)&lr, 0);
  while (true) {
    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return true;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchRecord(br, bFind);
    BranchRecord *br = bPage->GetRecordByPos(pos, true);
    uint32_t pageId = br->GetChildPageId();

    IndexPage *childPage = (IndexPage *)GetPage(
        pageId,
        page->GetPageLevel() == 1 ? PageType::LEAF_PAGE : PageType::BRANCH_PAGE,
        bWait);
    assert(childPage != nullptr);

    if (childPage->GetPageStatus() != PageStatus::VALID && !bWait) {
      if (childPage->PushWaitTask(ThreadPool::_currTask)) {
        page->ReadUnlock();
        page->DecRef();
        page = childPage;
        return false;
      }
    }

    if (bEdit && childPage->GetPageType() == PageType::LEAF_PAGE) {
      childPage->WriteLock();
    } else {
      childPage->ReadLock();
    }

    page->ReadUnlock();
    page->DecRef();
    page = childPage;
    if (page->GetPageStatus() != PageStatus::VALID) {
      return false;
    }
  }
}
} // namespace storage
