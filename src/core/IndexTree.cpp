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

IndexTree::IndexTree(const string &tableName, const string &fileName,
                     VectorDataValue &vctKey, VectorDataValue &vctVal,
                     IndexType iType) {
  _tableName = tableName;
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

      count = 0;
      for (IDataValue *dv : vctVal) {
        if (!dv->IsFixLength())
          count++;
      }
      _headPage->WriteValueVariableFieldCount(count);
    }

    StoragePool::WriteCachePage(_headPage);
    _rootPage = AllocateNewPage(HeadPage::PAGE_NULL_POINTER, 0);
    _rootPage->SetBeginPage(true);
    _rootPage->SetEndPage(true);
    StoragePool::WriteCachePage(_rootPage);
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

  _keyVarLen = _headPage->ReadKeyVariableFieldCount() * UI16_LEN;
  if (_headPage->ReadIndexType() == IndexType::PRIMARY) {
    _keyOffset = _keyVarLen + UI16_2_LEN + 1;
    _valVarLen = _headPage->ReadValueVariableFieldCount() * UI32_LEN;
    _valOffset = _valVarLen + (uint16_t)((_vctValue.size() + 7) >> 3);
  } else {
    _keyOffset = _keyVarLen + UI16_2_LEN;
    _valVarLen = _headPage->ReadValueVariableFieldCount() * UI16_LEN;
    _valOffset = _valVarLen + UI16_2_LEN;
  }

  _garbageOwner = new GarbageOwner(this);
  LOG_DEBUG << "Open index tree " << tableName;
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
  if (_headPage->IsDirty()) {
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

  LOG_DEBUG << "Close index tree " << _tableName;
}

void IndexTree ::Close() {
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
  root->DecRef();
  StoragePool::WriteCachePage(_headPage);
}

IndexPage *IndexTree::AllocateNewPage(PageID parentId, Byte pageLevel) {
  PageID newPageId = _garbageOwner == nullptr ? HeadPage::PAGE_NULL_POINTER
                                              : _garbageOwner->ApplyPage(1);
  if (newPageId == HeadPage::PAGE_NULL_POINTER)
    newPageId = _headPage->GetAndIncTotalPageCount();

  IndexPage *page = nullptr;
  if (0 != pageLevel) {
    page = new BranchPage(this, newPageId, pageLevel, parentId);
  } else {
    page = new LeafPage(this, newPageId, parentId);
  }

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

    ReadPageTask *task = new ReadPageTask(page);
    task->AddWaitTasks(ThreadPool::_currTask);
    ThreadPool::InstMain().AddTask(task);
  } else {
    page->DecRef();
  }
  _pageMutex.unlock();

  return page;
}

// LeafRecord *IndexTree::GetRecord(const RawKey &key) {
//   LeafPage *page = SearchRecursively(key);
//   LeafRecord *rr = page->GetRecord(key);
//
//   page->ReadUnlock();
//   page->DecRef();
//   return rr;
// }

// ErrorMsg *IndexTree::InsertRecord(LeafRecord *rr) {
//   LeafPage *page = SearchRecursively(*rr);
//   ErrorMsg *err = page->InsertRecord(rr);
//
//   if (page->GetTotalDataLength() > LeafPage::MAX_DATA_LENGTH * 3U) {
//     page->PageDivide();
//   }
//
//   page->WriteUnlock();
//   page->DecRef();
//   return err;
// }

// void IndexTree::GetRecords(const RawKey &key, VectorLeafRecord &vct) {
//   vct.reserve(256);
//   LeafPage *page = SearchRecursively(key);
//   while (true) {
//     VectorLeafRecord vctLr;
//     bool bLast = page->GetRecords(key, vctLr);
//     vct.insert(vct.end(), vctLr.begin(), vctLr.end());
//     vctLr.clear();
//
//     if (!bLast)
//       break;
//
//     uint32_t nextId = page->GetNextPageId();
//     if (nextId == HeadPage::PAGE_NULL_POINTER)
//       break;
//
//     LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
//     nextPage->ReadLock();
//     page->ReadUnlock();
//     page->DecRef();
//     page = nextPage;
//   }
//
//   page->ReadUnlock();
//   page->DecRef();
// }

// void IndexTree::QueryRecord(RawKey *keyStart, RawKey *keyEnd, bool bIncLeft,
//                             bool bIncRight, VectorLeafRecord &vctRt) {
//   vctRt.reserve(256);
//   LeafPage *page = nullptr;
//   uint32_t pos = 0;
//   if (keyStart == nullptr) {
//     uint32_t id = _headPage->ReadBeginLeafPagePointer();
//     page = (LeafPage *)GetPage(id, true);
//     page->ReadLock();
//   } else {
//     page = SearchRecursively(*keyStart);
//     bool bFind;
//     pos = page->SearchKey(*keyStart, bFind);
//   }
//
//   bool bStart = true;
//   while (true) {
//     VectorLeafRecord vct;
//     page->GetAllRecords(vct);
//     for (uint32_t i = 0; i < pos; i++) {
//       vct[i]->ReleaseRecord();
//     }
//
//     if (keyStart != nullptr && bStart && !bIncLeft) {
//       for (; pos < vct.size(); pos++) {
//         if (vct[pos]->CompareKey(*keyStart) != 0)
//           break;
//
//         vct[pos]->ReleaseRecord();
//       }
//
//       if (pos >= vct.size()) {
//         uint32_t idNext = page->GetNextPageId();
//         if (idNext == HeadPage::PAGE_NULL_POINTER) {
//           break;
//         }
//         LeafPage *pageNext = (LeafPage *)GetPage(idNext, true);
//         pageNext->ReadLock();
//         page->ReadUnlock();
//         page->DecRef();
//         page = pageNext;
//         pos = 0;
//         continue;
//       } else {
//         bStart = false;
//       }
//     }
//
//     int count = 0;
//     for (; pos < vct.size(); pos++) {
//       LeafRecord *rr = vct[pos];
//       if (keyEnd != nullptr) {
//         int hr = rr->CompareKey(*keyEnd);
//         if ((!bIncRight && hr >= 0) || hr > 0) {
//           break;
//         }
//       }
//
//       count++;
//       vctRt.push_back(rr);
//     }
//
//     if (pos < vct.size()) {
//       for (; pos < vct.size(); pos++) {
//         vct[pos]->ReleaseRecord();
//       }
//
//       vct.clear();
//       break;
//     }
//
//     vct.clear();
//     uint32_t nextId = page->GetNextPageId();
//     if (nextId == HeadPage::PAGE_NULL_POINTER)
//       break;
//
//     LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
//     nextPage->ReadLock();
//     page->ReadUnlock();
//     page->DecRef();
//     page = nextPage;
//     pos = 0;
//   }
//
//   page->ReadUnlock();
//   page->DecRef();
// }

/**
 * @brief
 */
bool IndexTree::SearchRecursively(const RawKey &key, bool bEdit,
                                  IndexPage *&page) {
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
    if (!childPage->IsValidPage()) {
      return false;
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
                                  IndexPage *&page) {
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

  BranchRecord br(this, (RawRecord *)&lr, 0);
  while (true) {
    if (page->GetPageType() == PageType::LEAF_PAGE) {
      return (LeafPage *)page;
    }

    BranchPage *bPage = (BranchPage *)page;
    bool bFind;
    uint32_t pos = bPage->SearchRecord(br, bFind);
    BranchRecord *br = bPage->GetRecordByPos(pos, true);
    uint32_t pageId = br->GetChildPageId();
    br->ReleaseRecord();

    IndexPage *childPage =
        (IndexPage *)GetPage(pageId, page->GetPageLevel() == 0);
    assert(childPage != nullptr);

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

// bool IndexTree::ReadRecord(const RawKey &key, uint64_t verStamp,
//                            VectorDataValue &vctVal, bool bOvf) {
//   assert(_headPage->ReadIndexType() == IndexType::PRIMARY);
//   LeafPage *page = SearchRecursively(key);
//   LeafRecord *rr = page->GetRecord(key);
//   bool b = false;
//   if (rr != nullptr) {
//     int hr = rr->GetListValue(vctVal, verStamp);
//
//     rr->ReleaseRecord();
//   }
//
//   page->ReadUnlock();
//   page->DecRef();
//   return b;
// }

// bool IndexTree::ReadPrimaryKeys(const RawKey &key, VectorRawKey &vctKey) {
//   assert(_headPage->ReadIndexType() != IndexType::PRIMARY);
//   if (vctKey.size() > 0)
//     vctKey.RemoveAll();
//   vctKey.reserve(256);
//   LeafPage *page = SearchRecursively(key);
//   VectorLeafRecord vctLr;
//   bool bLast = page->GetRecords(key, vctLr);
//
//   while (true) {
//     for (LeafRecord *lr : vctLr) {
//       vctKey.push_back(lr->GetPrimayKey());
//       lr->ReleaseRecord();
//     }
//
//     if (!bLast || _headPage->ReadIndexType() != IndexType::NON_UNIQUE)
//       break;
//
//     uint32_t nextId = page->GetNextPageId();
//     if (nextId == HeadPage::PAGE_NULL_POINTER)
//       break;
//
//     LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
//     nextPage->ReadLock();
//     page->ReadUnlock();
//     page->DecRef();
//     page = nextPage;
//     bLast = page->GetRecords(key, vctLr, true);
//   }
//   page->ReadUnlock();
//   page->DecRef();
//
//   return true;
// }

// void IndexTree::QueryIndex(const RawKey *keyStart, const RawKey *keyEnd,
//                            bool bIncLeft, bool bIncRight,
//                            VectorRawKey &vctKey) {
//   vctKey.clear();
//   vctKey.reserve(1024);
//   LeafPage *page = nullptr;
//   uint32_t pos = 0;
//   if (keyStart == nullptr) {
//     uint32_t id = _headPage->ReadBeginLeafPagePointer();
//     page = (LeafPage *)GetPage(id, true);
//     page->ReadLock();
//   } else {
//     page = SearchRecursively(*keyStart);
//   }
//
//   VectorLeafRecord vctLr;
//   bool bLast = page->FetchRecords(keyStart, keyEnd, bIncLeft, bIncRight,
//   vctLr);
//
//   while (true) {
//     for (LeafRecord *lr : vctLr) {
//       vctKey.push_back(lr->GetPrimayKey());
//     }
//     vctLr.RemoveAll();
//     if (!bLast)
//       break;
//
//     uint32_t nextId = page->GetNextPageId();
//     if (nextId == HeadPage::PAGE_NULL_POINTER)
//       break;
//
//     LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
//     nextPage->ReadLock();
//     page->ReadUnlock();
//     page->DecRef();
//     page = nextPage;
//
//     bLast = page->FetchRecords((vctKey.size() == 0 ? keyStart : nullptr),
//                                keyEnd, bIncLeft, bIncRight, vctLr);
//   }
//
//   page->ReadUnlock();
//   page->DecRef();
// }

// void IndexTree::QueryIndex(const RawKey *keyStart, const RawKey *keyEnd,
//                            bool bIncLeft, bool bIncRight, VectorRow &vctRow)
//                            {
//   vctRow.clear();
//   vctRow.reserve(1024);
//   LeafPage *page = nullptr;
//   if (keyStart == nullptr) {
//     uint32_t id = _headPage->ReadBeginLeafPagePointer();
//     page = (LeafPage *)GetPage(id, true);
//     page->ReadLock();
//   } else {
//     page = SearchRecursively(*keyStart);
//   }
//
//   VectorLeafRecord vctLr;
//   bool bLast = page->FetchRecords(keyStart, keyEnd, bIncLeft, bIncRight,
//   vctLr);
//
//   while (true) {
//     for (LeafRecord *lr : vctLr) {
//       VectorDataValue *vctDv = new VectorDataValue;
//       lr->GetListValue(*vctDv);
//       vctRow.push_back(vctDv);
//     }
//
//     vctLr.RemoveAll();
//     if (!bLast)
//       break;
//
//     uint32_t nextId = page->GetNextPageId();
//     if (nextId == HeadPage::PAGE_NULL_POINTER)
//       break;
//
//     LeafPage *nextPage = (LeafPage *)GetPage(nextId, true);
//     nextPage->ReadLock();
//     page->ReadUnlock();
//     page->DecRef();
//     page = nextPage;
//
//     bLast = page->FetchRecords((vctRow.size() == 0 ? keyStart : nullptr),
//                                keyEnd, bIncLeft, bIncRight, vctLr);
//   }
//
//   page->ReadUnlock();
//   page->DecRef();
// }
} // namespace storage
