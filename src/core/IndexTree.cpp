#include "IndexTree.h"
#include "../pool/PageBufferPool.h"
#include "../pool/StoragePool.h"
#include "../utils/Log.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexPage.h"
#include "LeafPage.h"
#include <shared_mutex>

namespace storage {
unordered_set<uint32_t> IndexTree::_setFiledId;
uint32_t IndexTree::_currFiledId = 1;
SpinMutex IndexTree::_fileIdMutex;

IndexTree::IndexTree(const string &indexName, const string &fileName,
                     VectorDataValue &vctKey, VectorDataValue &vctVal,
                     IndexType iType) {
  _indexName = indexName;
  _fileName = fileName;
  for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
    if (*iter == '\\')
      *iter = '/';
  }

  {
    lock_guard<SpinMutex> lock(_fileIdMutex);
    while (true) {
      if (_setFiledId.find(_currFiledId) == _setFiledId.end()) {
        _setFiledId.insert(_currFiledId);
        _fileId = _currFiledId;
        break;
      }
      _currFiledId++;
    }
  }

  _headPage = new HeadPage(this);
  if (iType != IndexType::UNKNOWN) {
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
      _headPage->SetPageLoaded();

      count = 0;
      for (IDataValue *dv : vctVal) {
        if (!dv->IsFixLength())
          count++;
      }
      _headPage->WriteValueVariableFieldCount(count);
    }

    StoragePool::WriteCachePage(_headPage, false);
    _rootPage = AllocateNewPage(PAGE_NULL_POINTER, 0);
    _rootPage->SetBeginPage(true);
    _rootPage->SetEndPage(true);
    StoragePool::WriteCachePage(_rootPage, true);
  } else {
    _headPage->ReadPage();
    FileVersion &&fv = _headPage->ReadFileVersion();
    if (!(fv == CURRENT_FILE_VERSION)) {
      throw ErrorMsg(TB_ERROR_INDEX_VERSION, {_fileName});
    }

    uint32_t rootId = _headPage->ReadRootPagePointer();
    _rootPage = GetPage(rootId, rootId == 0);

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
  }

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);

  //_keyVarLen = _headPage->ReadKeyVariableFieldCount() * UI16_LEN;
  //_keyOffset = _keyVarLen + UI16_2_LEN;
  if (_headPage->ReadIndexType() == IndexType::PRIMARY) {
    _valVarLen = _headPage->ReadValueVariableFieldCount() * UI32_LEN;
    _valOffset = _valVarLen + (uint16_t)((_vctValue.size() + 7) >> 3);
  } else {
    _valVarLen = 0;
    _valOffset = 0;
  }

  _garbageOwner = new GarbageOwner(this);
  LOG_DEBUG << "Open index tree " << indexName;
}

IndexTree::~IndexTree() {
  _bClosed = true;
  unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
  if (_rootPage != nullptr) {
    _rootPage->DecRef();
    _rootPage = nullptr;
  }

  while (_pagesInMem.load() > 1) {
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

  unique_lock<SpinMutex> lock2(_fileIdMutex);
  _setFiledId.erase(_fileId);

  if (_funcDestory != nullptr) {
    _funcDestory();
  }
  LOG_DEBUG << "Close index tree " << _indexName;
}

void IndexTree ::Close(function<void()> funcDestory) {
  _funcDestory = funcDestory;
  _bClosed = true;
  unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
  if (_rootPage != nullptr) {
    _rootPage->DecRef();
    _rootPage = nullptr;
  }
}

void IndexTree::CloneKeys(VectorDataValue &vct) {
  vct.RemoveAll();
  vct.reserve(_vctKey.size());
  for (IDataValue *dv : _vctKey) {
    vct.push_back(dv->Clone(false));
  }
}

void IndexTree::CloneValues(VectorDataValue &vct) {
  vct.RemoveAll();
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
      return new PageFile(_fileName);
    }
  }
}

void IndexTree::UpdateRootPage(IndexPage *root) {
  unique_lock<SharedSpinMutex> lock(_rootSharedMutex);
  _rootPage->DecRef();
  _headPage->WriteRootPagePointer(root->GetPageId());
  _rootPage = root;
  root->IncRef();
  StoragePool::WriteCachePage(_headPage, false);
}

IndexPage *IndexTree::AllocateNewPage(PageID parentId, Byte pageLevel) {
  PageID newPageId = _garbageOwner == nullptr ? PAGE_NULL_POINTER
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
  page->SetPageLoaded();
  PageBufferPool::AddPage(page);
  IncPages();

  LOG_DEBUG << "Allocate new CachePage, pageLevel=" << (int)pageLevel
            << "  pageId=" << newPageId;
  return page;
}

IndexPage *IndexTree::GetPage(PageID pageId, bool bLeafPage) {
  assert(pageId < _headPage->ReadTotalPageCount());
  IndexPage *page = (IndexPage *)PageBufferPool::GetPage(_fileId, pageId);
  if (page != nullptr)
    return page;

  _pageMutex.lock();
  page = (IndexPage *)PageBufferPool::GetPage(_fileId, pageId);
  if (page == nullptr) {
    if (bLeafPage) {
      page = new LeafPage(this, pageId);
    } else {
      page = new BranchPage(this, pageId);
    }

    PageBufferPool::AddPage(page);
    IncPages();

    if (Configure::GetDiskType() == DiskType::SSD) {
      page->ReadPage(nullptr);

      if (page->GetPageStatus() == PageStatus::VALID) {
        page->Init();
      } else {
        // In following time will add code to fix page;
        abort();
      }
    } else {
      ReadPageTask *task = new ReadPageTask(page);
      ThreadPool::InstMain().AddTask(task);
    }
  }

  _pageMutex.unlock();
  return page;
}

/**
 * @brief
 */
bool IndexTree::SearchRecursively(const RawKey &key, bool bEdit,
                                  IndexPage *&page, Task *task,
                                  bool bWait = false) {
  if (page != nullptr) {
    if (bEdit && childPage->GetPageType() == PageType::LEAF_PAGE) {
      page->WriteLock();
    } else {
      page->ReadLock();
    }
    page->RemoveTask(task);
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
    br->ReleaseRecord();

    IndexPage *childPage =
        (IndexPage *)GetPage(pageId, page->GetPageLevel() == 1);
    assert(childPage != nullptr);

    if (childPage->GetPageStatus() != PageStatus::VALID && !bWait) {
      if (childPage->PushWaitTask(task)) {
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
                                  IndexPage *&page, Task *task,
                                  bool bWait = false) {
  if (page != nullptr) {
    if (bEdit && childPage->GetPageType() == PageType::LEAF_PAGE) {
      page->WriteLock();
    } else {
      page->ReadLock();
    }
    page->RemoveTask(task);
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
    br->ReleaseRecord();

    IndexPage *childPage =
        (IndexPage *)GetPage(pageId, page->GetPageLevel() == 1);
    assert(childPage != nullptr);

    if (childPage->GetPageStatus() != PageStatus::VALID && !bWait) {
      if (childPage->PushWaitTask(task)) {
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
