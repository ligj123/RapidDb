#include "IndexTree.h"
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
void IndexTree::TestCloseWait(IndexTree *indexTree) {
  atomic_bool atomic{false};
  indexTree->Close([&atomic]() { atomic.store(true, memory_order_relaxed); });
  while (PageBufferPool::GetCacheSize() > 0) {
    this_thread::sleep_for(1ms);
    PageDividePool::AddTimerTask();
    StoragePool::AddTimerTask();
    PageBufferPool::AddTimerTask();
  }
  while (!atomic.load(memory_order_relaxed)) {
    std::this_thread::yield();
  }
}

bool IndexTree::CreateIndex(const MString &indexName, const MString &fileName,
                            VectorDataValue &vctKey, VectorDataValue &vctVal,
                            uint32_t indexId, IndexType iType) {
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
    unique_lock<ReentrantSharedSpinMutex> lock(_headPage->GetLock());
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
  PageDividePool::AddPage(_rootPage, true);

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

bool IndexTree::InitIndex(const MString &indexName, const MString &fileName,
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

  _headPage->ReadPage();
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
  for (IDataValue *dv : vctKey) {
    if (!dv->IsFixLength())
      count++;
  }
  assert(count == _headPage->ReadKeyVariableFieldCount());

  count = 0;
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
    _headPage->WritePage();
  }
  delete _headPage;
  _headPage = nullptr;

  while (_queueMutex.size() > 0) {
    delete _queueMutex.front();
    _queueMutex.pop();
  }

  for (auto iter = _mapMutex.begin(); iter != _mapMutex.end(); iter++) {
    delete iter->second;
  }
  _mapMutex.clear();

  while (_fileQueue.size() > 0) {
    delete _fileQueue.front();
    _fileQueue.pop();
  }

  if (_funcDestory != nullptr) {
    _funcDestory();
  }
  LOG_DEBUG << "Close index tree " << _indexName;
}

void IndexTree ::Close(function<void()> funcDestory) {
  unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
  _funcDestory = funcDestory;
  _bClosed = true;
  if (_rootPage != nullptr) {
    _rootPage->DecRef();
    _rootPage = nullptr;
  }
}

void IndexTree::CloneKeys(VectorDataValue &vct) {
  vct.clear();
  vct.reserve(_vctKey.size());
  for (IDataValue *dv : _vctKey) {
    vct.push_back(dv->Clone(false));
  }
}

void IndexTree::CloneValues(VectorDataValue &vct) {
  vct.clear();
  vct.reserve(_vctValue.size());
  for (IDataValue *dv : _vctValue) {
    vct.push_back(dv->Clone(false));
  }
}

PageFile *IndexTree::ApplyPageFile() {
  while (true) {
    unique_lock<SpinMutex> lock(_fileMutex);
    _fileCv.wait_for(lock, 1ms, [this] {
      return _rpfCount < Configure::GetMaxPageFileCount();
    });

    if (_fileQueue.size() > 0) {
      PageFile *rpf = _fileQueue.front();
      _fileQueue.pop();
      return rpf;
    } else if (_rpfCount < Configure::GetMaxPageFileCount()) {
      _rpfCount++;
      return new PageFile(_fileName.c_str());
    }
  }
}

void IndexTree::UpdateRootPage(IndexPage *root) {
  unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
  _headPage->WriteRootPagePointer(root->GetPageId());
  if (_rootPage != nullptr) {
    _rootPage->DecRef();
    _rootPage = root;
    root->IncRef();
  }
  StoragePool::AddPage(_headPage, false);
}

IndexPage *IndexTree::AllocateNewPage(PageID parentId, Byte pageLevel) {
  PageID newPageId = (_garbageOwner == nullptr) ? PAGE_NULL_POINTER
                                                : _garbageOwner->ApplyPage(1);
  if (newPageId == PAGE_NULL_POINTER)
    newPageId = _headPage->GetAndIncTotalPageCount();

  IndexPage *page = nullptr;
  if (0 != pageLevel) {
    page = new BranchPage(this, newPageId, pageLevel, parentId);
  } else {
    page = new LeafPage(this, newPageId, parentId);
  }

  page->SetPageStatus(PageStatus::VALID);
  page->GetBysPage()[IndexPage::PAGE_BEGIN_END_OFFSET] = 0;
  PageBufferPool::AddPage(page);
  IncPages();

  LOG_DEBUG << "Allocate new CachePage, pageLevel=" << (int)pageLevel
            << "  pageId=" << newPageId;
  return page;
}

IndexPage *IndexTree::GetPage(PageID pageId, PageType type, bool wait) {
  assert(pageId < _headPage->ReadTotalPageCount());
  IndexPage *page = (IndexPage *)PageBufferPool::GetPage(_fileId, pageId);
  if (page != nullptr) {
    if (wait)
      page->WaitRead();
    return page;
  }

  _pageMutex.lock();
  page = (IndexPage *)PageBufferPool::GetPage(_fileId, pageId);
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
    _pageMutex.unlock();

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
  } else {
    _pageMutex.unlock();
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
    BranchRecord *br = bPage->GetRecordByPos(pos, true);
    uint32_t pageId = ((BranchRecord *)br)->GetChildPageId();

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
