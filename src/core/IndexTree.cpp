#include "IndexTree.h"
#include "../pool/PageBufferPool.h"
#include "../pool/StoragePool.h"
#include "../utils/Log.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexPage.h"
#include <shared_mutex>

namespace storage {
unordered_set<uint16_t> IndexTree::_setFiledId;
uint16_t IndexTree::_currFiledId = 0;
utils::SpinMutex IndexTree::_spinMutex;

IndexTree::IndexTree(const string &tableName, const string &fileName,
                     VectorDataValue &vctKey, VectorDataValue &vctVal) {
  _tableName = tableName;
  _fileName = fileName;
  for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
    if (*iter == '\\')
      *iter = '/';
  }

  {
    lock_guard<utils::SpinMutex> lock(_spinMutex);
    while (true) {
      if (_setFiledId.find(_currFiledId) == _setFiledId.end()) {
        _setFiledId.insert(_currFiledId);
        _fileId = _currFiledId;
        break;
      }
      _currFiledId++;
    }
  }

  fstream fs(_fileName);
  bool bExist = fs.good();
  _headPage = new HeadPage(this);
  if (!bExist) {
    {
      unique_lock<utils::ReentrantSharedSpinMutex> lock(_headPage->GetLock());
      memset(_headPage->GetBysPage(), 0, Configure::GetDiskClusterSize());
      _headPage->WriteFileVersion();
      _headPage->WriteRootPagePointer(0);
      _headPage->WriteTotalPageCount(0);
      _headPage->WriteTotalRecordCount(0);
      _headPage->WriteAutoPrimaryKey(0);
    }

    StoragePool::WriteCachePage(_headPage);
    _rootPage = AllocateNewPage(HeadPage::NO_PARENT_POINTER, 0);
  } else {
    _headPage->ReadPage();
    FileVersion &&fv = _headPage->ReadFileVersion();
    if (!(fv == INDEX_FILE_VERSION)) {
      throw ErrorMsg(TB_ERROR_INDEX_VERSION, {_fileName});
    }

    uint64_t rootId = _headPage->ReadRootPagePointer();
    _rootPage = GetPage(rootId, rootId == 0);
  }

  _vctKey.swap(vctKey);
  _vctValue.swap(vctVal);
  LOG_DEBUG << "Open index tree " << tableName;
}

IndexTree::~IndexTree() {
  unique_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
  if (_headPage->IsDirty()) {
    _headPage->WritePage();
  }

  while (_queueMutex.size() > 0) {
    delete _queueMutex.front();
    _queueMutex.pop();
  }

  for (auto iter = _mapMutex.begin(); iter != _mapMutex.end(); iter++) {
    delete iter->second;
  }

  if (_ovfFile != nullptr)
    delete _ovfFile;
  while (_fileQueue.size() > 0) {
    delete _fileQueue.front();
    _fileQueue.pop();
  }

  delete _headPage;
}

void IndexTree::Close(bool bWait) {
  _bClosed = true;
  while (_taskWaiting.load() > 1) {
    this_thread::sleep_for(chrono::milliseconds(1));
  }

  unique_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
  _rootPage->DecRefCount();
  if (!bWait)
    return;

  if (_headPage->IsDirty()) {
    _headPage->WritePage();
  }

  while (_queueMutex.size() > 0) {
    delete _queueMutex.front();
    _queueMutex.pop();
  }

  for (auto iter = _mapMutex.begin(); iter != _mapMutex.end(); iter++) {
    delete iter->second;
  }
  _mapMutex.clear();

  if (_ovfFile != nullptr)
    delete _ovfFile;
  while (_fileQueue.size() > 0) {
    delete _fileQueue.front();
    _fileQueue.pop();
  }
}

void IndexTree::CloneKeys(VectorDataValue &vct) {
  vct.RemoveAll();
  vct.reserve(_vctKey.size());
  for (IDataValue *dv : _vctKey) {
    vct.push_back(dv->CloneDataValue(false));
  }
}

void IndexTree::CloneValues(VectorDataValue &vct) {
  vct.RemoveAll();
  vct.reserve(_vctValue.size());
  for (IDataValue *dv : _vctValue) {
    vct.push_back(dv->CloneDataValue(false));
  }
}

PageFile *IndexTree::ApplyPageFile() {
  while (true) {
    {
      lock_guard<utils::SpinMutex> lock(_fileMutex);
      if (_fileQueue.size() > 0) {
        PageFile *rpf = _fileQueue.front();
        _fileQueue.pop();
        return rpf;
      } else if (_rpfCount < Configure::GetMaxPageFileCount()) {
        _rpfCount++;
        return new PageFile(_fileName);
      }
    }

    this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

utils::ErrorMsg *IndexTree::InsertRecord(LeafRecord *rr) {
  LeafPage *page = SearchRecursively(*rr);
  utils::ErrorMsg *err = page->InsertRecord(rr);

  if (page->GetTotalDataLength() > LeafPage::MAX_DATA_LENGTH * 3U) {
    page->PageDivide();
  }

  page->WriteUnlock();
  page->DecRefCount();
  return err;
}

IndexPage *IndexTree::GetPage(uint64_t pageId, bool bLeafPage) {
  assert(pageId < _headPage->ReadTotalPageCount());
  IndexPage *page = PageBufferPool::GetPage(_fileId, pageId);
  if (page != nullptr)
    return page;

  _pageMutex.lock();

  utils::SpinMutex *lockPage = nullptr;
  auto iter = _mapMutex.find(pageId);
  if (iter != _mapMutex.end()) {
    lockPage = iter->second;
  } else {
    lockPage = new utils::SpinMutex;
    _mapMutex.insert(pair<uint64_t, utils::SpinMutex *>(pageId, lockPage));
  }

  _pageMutex.unlock();
  lockPage->lock();

  page = PageBufferPool::GetPage(_fileId, pageId);
  if (page == nullptr) {
    if (bLeafPage) {
      page = new LeafPage(this, pageId);
    } else {
      page = new BranchPage(this, pageId);
    }

    std::future<int> fut = StoragePool::ReadCachePage(page);
    fut.get();

    page->Init();
    PageBufferPool::AddPage(page);
    page->IncRefCount();
    IncPages();
  }

  _pageMutex.lock();
  _queueMutex.push(lockPage);
  _mapMutex.erase(pageId);
  lockPage->unlock();
  _pageMutex.unlock();

  return page;
}

void IndexTree::UpdateRootPage(IndexPage *root) {
  unique_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
  _rootPage->DecRefCount();
  _headPage->WriteRootPagePointer(root->GetPageId());
  _rootPage = root;
  root->IncRefCount();
  StoragePool::WriteCachePage(_headPage);
}

LeafRecord *IndexTree::GetRecord(const RawKey &key) {
  LeafPage *page = SearchRecursively(key);
  LeafRecord *rr = page->GetRecord(key);

  page->ReadUnlock();
  page->DecRefCount();
  return rr;
}

void IndexTree::GetRecords(const RawKey &key, VectorLeafRecord &vct) {
  vct.reserve(256);
  LeafPage *page = SearchRecursively(key);
  while (true) {
    VectorLeafRecord vctLr;
    page->GetRecords(key, vctLr);
    vct.insert(vct.end(), vctLr.begin(), vctLr.end());

    LeafRecord *lastLr = page->GetLastRecord();
    if (lastLr != nullptr && lastLr->CompareKey(key) > 0) {
      lastLr->ReleaseRecord();
      break;
    }

    if (lastLr != nullptr) {
      lastLr->ReleaseRecord();
    }

    uint64_t nextId = page->GetNextPageId();
    if (nextId == HeadPage::NO_NEXT_PAGE_POINTER)
      break;

    LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
    nextPage->ReadLock();
    page->ReadUnlock();
    page->DecRefCount();
    page = nextPage;
  }

  page->ReadUnlock();
  page->DecRefCount();
}

void IndexTree::QueryRecord(RawKey *keyStart, RawKey *keyEnd, bool bIncLeft,
                            bool bIncRight, VectorLeafRecord &vctRt) {
  vctRt.reserve(256);
  LeafPage *page = nullptr;
  uint32_t pos = 0;
  if (keyStart == nullptr) {
    uint64_t id = _headPage->ReadBeginLeafPagePointer();
    page = (LeafPage *)GetPage(id, true);
    page->ReadLock();
  } else {
    page = SearchRecursively(*keyStart);
    bool bFind;
    pos = page->SearchKey(*keyStart, bFind);
  }

  bool bStart = true;
  while (true) {
    VectorLeafRecord vct;
    page->GetAllRecords(vct);
    for (uint32_t i = 0; i < pos; i++) {
      vct[i]->ReleaseRecord();
    }

    if (keyStart != nullptr && bStart && !bIncLeft) {
      for (; pos < vct.size(); pos++) {
        if (vct[pos]->CompareKey(*keyStart) != 0)
          break;

        vct[pos]->ReleaseRecord();
      }

      if (pos >= vct.size()) {
        uint64_t idNext = page->GetNextPageId();
        if (idNext == HeadPage::NO_NEXT_PAGE_POINTER) {
          break;
        }
        LeafPage *pageNext = (LeafPage *)GetPage(idNext, true);
        pageNext->ReadLock();
        page->ReadUnlock();
        page->DecRefCount();
        page = pageNext;
        pos = 0;
        continue;
      } else {
        bStart = false;
      }
    }

    int count = 0;
    for (; pos < vct.size(); pos++) {
      LeafRecord *rr = vct[pos];
      if (keyEnd != nullptr) {
        int hr = rr->CompareKey(*keyEnd);
        if ((!bIncRight && hr >= 0) || hr > 0) {
          break;
        }
      }

      count++;
      vctRt.push_back(rr);
    }

    if (pos < vct.size()) {
      for (; pos < vct.size(); pos++) {
        vct[pos]->ReleaseRecord();
      }

      vct.clear();
      break;
    }

    vct.clear();
    uint64_t nextId = page->GetNextPageId();
    if (nextId == HeadPage::NO_PARENT_POINTER)
      break;

    LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
    nextPage->ReadLock();
    page->ReadUnlock();
    page->DecRefCount();
    page = nextPage;
    pos = 0;
  }

  page->ReadUnlock();
  page->DecRefCount();
}

IndexPage *IndexTree::AllocateNewPage(uint64_t parentId, Byte pageLevel) {
  uint64_t newPageId = _headPage->GetAndIncTotalPageCount();
  IndexPage *page = nullptr;
  if (0 != pageLevel) {
    page = new BranchPage(this, newPageId, pageLevel, parentId);
  } else {
    page = new LeafPage(this, newPageId, parentId);
  }

  PageBufferPool::AddPage(page);
  page->IncRefCount();
  IncPages();

  LOG_DEBUG << "Allocate new CachePage, pageLevel=" << (int)pageLevel
            << "  pageId=" << newPageId;
  return page;
}

LeafPage *IndexTree::SearchRecursively(const RawKey &key) {
  IndexPage *page = nullptr;
  while (true) {
    {
      std::shared_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
      bool b = _rootPage->ReadTryLock();

      if (b) {
        page = _rootPage;
        page->IncRefCount();

        if (typeid(*page) == typeid(LeafPage))
          return (LeafPage *)page;
        else
          break;
      }
    }

    this_thread::sleep_for(std::chrono::microseconds(1));
  }

  while (true) {
    if (typeid(*page) == typeid(LeafPage)) {
      return (LeafPage *)page;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchKey(key, bFind);
    BranchRecord *br = bPage->GetRecordByPos(pos, true);
    uint64_t pageId = ((BranchRecord *)br)->GetChildPageId();
    br->ReleaseRecord();

    IndexPage *childPage =
        (IndexPage *)GetPage(pageId, page->GetPageLevel() == 1);
    assert(childPage != nullptr);

    childPage->ReadLock();
    page->ReadUnlock();
    page->DecRefCount();
    page = childPage;
  }
}

LeafPage *IndexTree::SearchRecursively(const LeafRecord &lr) {
  IndexPage *page = nullptr;
  while (true) {
    {
      std::shared_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
      bool b = false;
      if (typeid(*_rootPage) == typeid(LeafPage)) {
        b = _rootPage->WriteTryLock();
      } else {
        b = _rootPage->ReadTryLock();
      }

      if (b) {
        page = _rootPage;
        page->IncRefCount();
        if (typeid(*page) == typeid(LeafPage))
          return (LeafPage *)page;
        else
          break;
      }
    }

    this_thread::sleep_for(std::chrono::microseconds(1));
  }

  BranchRecord br(this, (RawRecord *)&lr, 0);
  while (true) {
    if (typeid(*page) == typeid(LeafPage)) {
      return (LeafPage *)page;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchRecord(br, bFind);
    BranchRecord *br = bPage->GetRecordByPos(pos, true);
    uint64_t pageId = ((BranchRecord *)br)->GetChildPageId();
    br->ReleaseRecord();

    IndexPage *childPage =
        (IndexPage *)GetPage(pageId, page->GetPageLevel() == 1);
    assert(childPage != nullptr);

    if (typeid(*childPage) == typeid(LeafPage)) {
      childPage->WriteLock();
    } else {
      childPage->ReadLock();
    }

    page->ReadUnlock();
    page->DecRefCount();
    page = childPage;
  }
}

bool IndexTree::ReadRecord(const RawKey &key, uint64_t verStamp,
                           VectorDataValue &vctVal, bool bOvf) {
  assert(_headPage->ReadIndexType() == IndexType::PRIMARY);
  LeafPage *page = SearchRecursively(key);
  LeafRecord *rr = page->GetRecord(key);
  bool b = false;
  if (rr != nullptr) {
    int hr = rr->GetListValue(vctVal, verStamp);
    if (bOvf && hr > 0) {
      rr->GetListOverflow(vctVal);
    }
    rr->ReleaseRecord();
  }

  page->ReadUnlock();
  page->DecRefCount();
  return b;
}

bool IndexTree::ReadRecordOverflow(const RawKey &key, VectorDataValue &vctVal) {
  assert(_headPage->ReadIndexType() == IndexType::PRIMARY);
  LeafPage *page = SearchRecursively(key);
  LeafRecord *rr = page->GetRecord(key);
  bool b = false;
  if (rr != nullptr) {
    rr->GetListOverflow(vctVal);
    b = true;
  }
  page->ReadUnlock();
  page->DecRefCount();
  return true;
}

bool IndexTree::ReadPrimaryKeys(const RawKey &key, VectorRawKey &vctKey) {
  assert(_headPage->ReadIndexType() != IndexType::PRIMARY);
  if (vctKey.size() > 0)
    vctKey.RemoveAll();
  vctKey.reserve(256);
  LeafPage *page = SearchRecursively(key);
  VectorLeafRecord vctLr;
  bool bLast = page->GetRecords(key, vctLr);

  while (true) {
    for (LeafRecord *lr : vctLr) {
      vctKey.push_back(lr->GetPrimayKey());
      lr->ReleaseRecord();
    }

    if (!bLast || _headPage->ReadIndexType() != IndexType::NON_UNIQUE)
      break;

    uint64_t nextId = page->GetNextPageId();
    if (nextId == HeadPage::NO_NEXT_PAGE_POINTER)
      break;

    LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
    nextPage->ReadLock();
    page->ReadUnlock();
    page->DecRefCount();
    page = nextPage;
    bLast = page->GetRecords(key, vctLr, true);
  }
  page->ReadUnlock();
  page->DecRefCount();

  return true;
}

void IndexTree::QueryIndex(const RawKey *keyStart, const RawKey *keyEnd,
                           bool bIncLeft, bool bIncRight,
                           VectorRawKey &vctKey) {
  vctKey.clear();
  vctKey.reserve(1024);
  LeafPage *page = nullptr;
  uint32_t pos = 0;
  if (keyStart == nullptr) {
    uint64_t id = _headPage->ReadBeginLeafPagePointer();
    page = (LeafPage *)GetPage(id, true);
    page->ReadLock();
  } else {
    page = SearchRecursively(*keyStart);
  }

  VectorLeafRecord vctLr;
  bool bLast = page->FetchRecords(keyStart, keyEnd, bIncLeft, bIncRight, vctLr);

  while (true) {
    for (LeafRecord *lr : vctLr) {
      vctKey.push_back(lr->GetPrimayKey());
    }
    vctLr.RemoveAll();
    if (!bLast)
      break;

    uint64_t nextId = page->GetNextPageId();
    if (nextId == HeadPage::NO_NEXT_PAGE_POINTER)
      break;

    LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
    nextPage->ReadLock();
    page->ReadUnlock();
    page->DecRefCount();
    page = nextPage;

    bLast = page->FetchRecords((vctKey.size() == 0 ? keyStart : nullptr),
                               keyEnd, bIncLeft, bIncRight, vctLr);
  }

  page->ReadUnlock();
  page->DecRefCount();
}

void IndexTree::QueryIndex(const RawKey *keyStart, const RawKey *keyEnd,
                           bool bIncLeft, bool bIncRight, VectorRow &vctRow) {
  vctRow.clear();
  vctRow.reserve(1024);
  LeafPage *page = nullptr;
  uint32_t pos = 0;
  if (keyStart == nullptr) {
    uint64_t id = _headPage->ReadBeginLeafPagePointer();
    page = (LeafPage *)GetPage(id, true);
    page->ReadLock();
  } else {
    page = SearchRecursively(*keyStart);
  }

  VectorLeafRecord vctLr;
  bool bLast = page->FetchRecords(keyStart, keyEnd, bIncLeft, bIncRight, vctLr);

  while (true) {
    for (LeafRecord *lr : vctLr) {
      VectorDataValue *vctDv = new VectorDataValue;
      lr->GetListValue(*vctDv);
      vctRow.push_back(vctDv);
    }

    vctLr.RemoveAll();
    if (!bLast)
      break;

    uint64_t nextId = page->GetNextPageId();
    if (nextId == HeadPage::NO_NEXT_PAGE_POINTER)
      break;

    LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
    nextPage->ReadLock();
    page->ReadUnlock();
    page->DecRefCount();
    page = nextPage;

    bLast = page->FetchRecords((vctRow.size() == 0 ? keyStart : nullptr),
                               keyEnd, bIncLeft, bIncRight, vctLr);
  }

  page->ReadUnlock();
  page->DecRefCount();
}
} // namespace storage
