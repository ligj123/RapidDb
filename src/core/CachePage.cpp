#include "CachePage.h"
#include "../cache/CachePool.h"
#include "IndexTree.h"
#include <boost/crc.hpp>

namespace storage {
static thread_local boost::crc_32_type crc32;
const uint32_t CachePage::CACHE_PAGE_SIZE =
    (uint32_t)Configure::GetCachePageSize();
const uint32_t CachePage::HEAD_PAGE_SIZE =
    (uint32_t)Configure::GetDiskClusterSize();
const uint32_t CachePage::CRC32_PAGE_OFFSET =
    (uint32_t)(Configure::GetCachePageSize() - sizeof(uint32_t));
const uint32_t CachePage::CRC32_HEAD_OFFSET =
    (uint32_t)(Configure::GetDiskClusterSize() - sizeof(uint32_t));

CachePage::CachePage(IndexTree *indexTree, PageID pageId, PageType type)
    : _indexTree(indexTree), _pageId(pageId), _pageType(type),
      _fileId(indexTree->GetFileId()) {
  if (type == PageType::HEAD_PAGE) {
    _bysPage = CachePool::Apply(HEAD_PAGE_SIZE);
  } else if (type != PageType::OVERFLOW_PAGE) {
    _bysPage = CachePool::ApplyPage();
  }
}

CachePage::~CachePage() {
  if (_pageType == PageType::HEAD_PAGE) {
    CachePool::Release(_bysPage, HEAD_PAGE_SIZE);
  } else if (_pageType != PageType::OVERFLOW_PAGE) {
    CachePool::ReleasePage(_bysPage);
  }
}

void CachePage::DecRef(int num) {
  if (_pageType == PageType::HEAD_PAGE)
    return;
  assert(num > 0);
  int32_t rc = _refCount.fetch_sub(num, memory_order_relaxed);

  assert(rc - num >= 0);
  if (rc - num == 0) {
    _indexTree->DecPages();
    delete this;
  }
}

void CachePage::ReadPage(PageFile *pageFile) {
  unique_lock<SpinMutex> lock(_pageLock);

  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  if (_pageId != PAGE_NULL_POINTER) {
    pFile->ReadPage(Configure::GetDiskClusterSize() +
                        _pageId * Configure::GetCachePageSize(),
                    (char *)_bysPage, (uint32_t)Configure::GetCachePageSize());

    if (_pageType != PageType::OVERFLOW_PAGE) {
      crc32.reset();
      crc32.process_bytes(_bysPage, CRC32_PAGE_OFFSET);
      if (crc32.checksum() != (uint32_t)ReadInt(CRC32_PAGE_OFFSET)) {
        _pageStatus = PageStatus::INVALID;
      } else {
        _pageStatus = PageStatus::VALID;
      }
    }
  } else {
    pFile->ReadPage(0, (char *)_bysPage,
                    (uint32_t)Configure::GetDiskClusterSize());
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
    if (crc32.checksum() != (uint32_t)ReadInt(CRC32_HEAD_OFFSET)) {
      _pageStatus = PageStatus::INVALID;
    } else {
      _pageStatus = PageStatus::VALID;
    }
  }
  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
  _bDirty = false;

  ThreadPool::InstMain().AddTasks(_waitTasks);
  _waitTasks.clear();
  lock.unlock();
  _pageCv.notify_all();
}

void CachePage::WritePage(PageFile *pageFile) {
  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  char *tmp = PageFile::_tmpBuff;
  {
    unique_lock<SpinMutex> lock(_pageLock);
    BytesCopy(
        tmp, _bysPage,
        (_pageId != PAGE_NULL_POINTER ? CACHE_PAGE_SIZE : HEAD_PAGE_SIZE));
    _bDirty = false;
  }

  if (_pageId != PAGE_NULL_POINTER) {
    if (_pageType != PageType::OVERFLOW_PAGE) {
      crc32.reset();
      crc32.process_bytes(_bysPage, CRC32_PAGE_OFFSET);
      *((int *)&tmp[CRC32_PAGE_OFFSET]) = crc32.checksum();
    }

    pFile->WritePage(Configure::GetDiskClusterSize() +
                         _pageId * Configure::GetCachePageSize(),
                     tmp, (uint32_t)Configure::GetCachePageSize());
  } else {
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
    *((int *)&tmp[CRC32_HEAD_OFFSET]) = crc32.checksum();

    pFile->WritePage(0, tmp, (uint32_t)Configure::GetDiskClusterSize());
  }
  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
}
} // namespace storage
