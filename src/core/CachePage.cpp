#include "CachePage.h"
#include "../cache/CachePool.h"
#include "IndexTree.h"
#include <boost/crc.hpp>

namespace storage {
static thread_local boost::crc_32_type crc32;
const uint32_t CachePage::CACHE_PAGE_SIZE =
    (uint32_t)Configure::GetIndexPageSize();
const uint32_t CachePage::HEAD_PAGE_SIZE =
    (uint32_t)Configure::GetDiskClusterSize();
const uint32_t CachePage::CRC32_PAGE_OFFSET =
    (uint32_t)(Configure::GetIndexPageSize() - sizeof(uint32_t));
const uint32_t CachePage::CRC32_HEAD_OFFSET =
    (uint32_t)(Configure::GetDiskClusterSize() - sizeof(uint32_t));

CachePage::CachePage(IndexTree *indexTree, PageID pageId, PageType type)
    : _indexTree(indexTree), _pageId(pageId), _pageType(type),
      _fileId(indexTree->GetFileId()) {
  if (type == PageType::HEAD_PAGE) {
    _bysPage = CachePool::Apply(HEAD_PAGE_SIZE);
  } else if (type == PageType::BRANCH_PAGE || type == PageType::LEAF_PAGE) {
    _bysPage = CachePool::ApplyPage();
  }
}

CachePage::~CachePage() {
  if (_pageType == PageType::HEAD_PAGE) {
    CachePool::Release(_bysPage, HEAD_PAGE_SIZE);
  } else if (type == PageType::BRANCH_PAGE || type == PageType::LEAF_PAGE) {
    CachePool::ReleasePage(_bysPage);
  }
}

void CachePage::AfterRead() {
  bool bvalid = true;
  if (_pageType == PageType::HEAD_PAGE) {
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
    if (crc32.checksum() != (uint32_t)ReadInt(CRC32_HEAD_OFFSET)) {
      bvalid = false;
    }
  } else if (_pageType != PageType::OVERFLOW_PAGE) {
    crc32.reset();
    crc32.process_bytes(_bysPage, CRC32_PAGE_OFFSET);
    if (crc32.checksum() != (uint32_t)ReadInt(CRC32_PAGE_OFFSET)) {
      bvalid = false;
    }
  }

  if (bvalid) {
    _bDirty = false;
    Init();
    _pageStatus = PageStatus::VALID;
  } else {
    _pageStatus = PageStatus::INVALID;
  }
}
} // namespace storage
