#include "CachePage.h"
#include "../cache/CachePool.h"
#include "IndexTree.h"

namespace storage {
const uint32_t CachePage::CRC32_INDEX_OFFSET =
    (uint32_t)(Configure::GetCachePageSize() - sizeof(uint64_t));
const uint32_t CachePage::CRC32_HEAD_OFFSET =
    (uint32_t)(Configure::GetDiskClusterSize() - sizeof(uint64_t));

CachePage::CachePage(IndexTree *indexTree, uint64_t pageId) {
  _indexTree = indexTree;
  _pageId = pageId;
  _fileId = _indexTree->GetFileId();
  if (pageId == UINT64_MAX) {
    _bysPage = new Byte[Configure::GetDiskClusterSize()];
  } else {
    _bysPage = CachePool::ApplyPage();
  }
}

CachePage::~CachePage() {
  if (_pageId == UINT64_MAX) {
    delete[] _bysPage;
  } else {
    CachePool::ReleasePage(_bysPage);
  }
}

void CachePage::IncRefCount() {
  // _indexTree->IncTask();
  // _refCount.fetch_add(1);
}

void CachePage::DecRefCount() {
  // _indexTree->DecTask();
  // _refCount.fetch_sub(1);
}

bool CachePage::IsFileClosed() const { return _indexTree->IsClosed(); }

void CachePage::ReadPage() {
  unique_lock<SpinMutex> lock(_pageLock);

  PageFile *pageFile = _indexTree->ApplyPageFile();
  if (_pageId != UINT64_MAX) {
    pageFile->ReadPage(Configure::GetDiskClusterSize() +
                           _pageId * Configure::GetCachePageSize(),
                       (char *)_bysPage,
                       (uint32_t)Configure::GetCachePageSize());
  } else {
    pageFile->ReadPage(0, (char *)_bysPage,
                       (uint32_t)Configure::GetDiskClusterSize());
  }
  _indexTree->ReleasePageFile(pageFile);
  _bDirty = false;
}

void CachePage::WritePage() {
  PageFile *pageFile = _indexTree->ApplyPageFile();
  unique_lock<SpinMutex> lock(_pageLock);

  if (_pageId != UINT64_MAX) {
    pageFile->WritePage(Configure::GetDiskClusterSize() +
                            _pageId * Configure::GetCachePageSize(),
                        (char *)_bysPage,
                        (uint32_t)Configure::GetCachePageSize());
  } else {
    pageFile->WritePage(0, (char *)_bysPage,
                        (uint32_t)Configure::GetDiskClusterSize());
  }
  _indexTree->ReleasePageFile(pageFile);
  _bDirty = false;
}
} // namespace storage
