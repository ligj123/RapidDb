#include "CachePage.h"
#include "../cache/CachePool.h"
#include "IndexTree.h"
#include <boost/crc.hpp>

namespace storage {
const uint32_t CachePage::CACHE_PAGE_SIZE =
    (uint32_t)Configure::GetCachePageSize();
const uint32_t CachePage::HEAD_PAGE_SIZE =
    (uint32_t)Configure::GetCachePageSize();
const uint32_t CachePage::CRC32_PAGE_OFFSET =
    (uint32_t)(Configure::GetCachePageSize() - sizeof(uint32_t));
const uint32_t CachePage::CRC32_HEAD_OFFSET =
    (uint32_t)(Configure::GetDiskClusterSize() - sizeof(uint32_t));

CachePage::CachePage(IndexTree *indexTree, PageID pageId, PageType type)
    : _indexTree(indexTree), _pageId(pageId), _pageType(type) {
  _fileId = _indexTree->GetFileId();
  if (pageId == HeadPage::PAGE_NULL_POINTER) {
    _bysPage = CachePool::Apply(HEAD_PAGE_SIZE);
  } else {
    _bysPage = CachePool::ApplyPage();
  }
}

CachePage::~CachePage() {
  if (_pageId == HeadPage::PAGE_NULL_POINTER) {
    CachePool::Release(_bysPage, HEAD_PAGE_SIZE);
  } else {
    CachePool::ReleasePage(_bysPage);
  }
}

void CachePage::ReadPage(PageFile *pageFile) {
  static thread_local boost::crc_32_type crc32;
  unique_lock<SpinMutex> lock(_pageLock);

  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  if (_pageId != HeadPage::PAGE_NULL_POINTER) {
    pFile->ReadPage(Configure::GetDiskClusterSize() +
                        _pageId * Configure::GetCachePageSize(),
                    (char *)_bysPage, (uint32_t)Configure::GetCachePageSize());

    if (_pageType != PageType::OVERFLOW_PAGE) {
      crc32.reset();
      crc32.process_bytes(_bysPage, CRC32_PAGE_OFFSET);
      if (crc32.checksum() != ReadInt(CRC32_PAGE_OFFSET)) {
        _bInvalidPage = true;
      }
    }
  } else {
    pFile->ReadPage(0, (char *)_bysPage,
                    (uint32_t)Configure::GetDiskClusterSize());
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
    if (crc32.checksum() != ReadInt(CRC32_HEAD_OFFSET)) {
      _bInvalidPage = true;
    }
  }
  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
  _bDirty = false;
}

void CachePage::WritePage(PageFile *pageFile) {
  static thread_local boost::crc_32_type crc32;
  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  char *tmp = PageFile::_ovfBuff.GetBuf();
  {
    unique_lock<SpinMutex> lock(_pageLock);
    BytesCopy(tmp, _bysPage,
              (_pageId != HeadPage::PAGE_NULL_POINTER
                   ? Configure::GetCachePageSize()
                   : Configure::GetDiskClusterSize()));
    _bDirty = false;
  }

  if (_pageId != HeadPage::PAGE_NULL_POINTER) {
    if (_pageType != PageType::OVERFLOW_PAGE) {
      crc32.reset();
      crc32.process_bytes(_bysPage, CRC32_PAGE_OFFSET);
      *((int *)tmp[CRC32_PAGE_OFFSET]) = crc32.checksum();
    }

    pFile->WritePage(Configure::GetDiskClusterSize() +
                         _pageId * Configure::GetCachePageSize(),
                     tmp, (uint32_t)Configure::GetCachePageSize());
  } else {
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
    *((int *)tmp[CRC32_HEAD_OFFSET]) = crc32.checksum();

    pFile->WritePage(0, tmp, (uint32_t)Configure::GetDiskClusterSize());
  }
  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
}
} // namespace storage
