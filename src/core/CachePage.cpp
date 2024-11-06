#include "CachePage.h"
#include "../cache/CachePool.h"
#include "IndexTree.h"
#include <boost/crc.hpp>

namespace storage {
const uint32_t CachePage::INDEX_PAGE_SIZE =
    (uint32_t)Configure::GetIndexPageSize();
const uint32_t CachePage::HEAD_PAGE_SIZE =
    (uint32_t)Configure::GetDiskClusterSize();
const uint32_t CachePage::CRC32_INDEX_OFFSET =
    (uint32_t)(Configure::GetIndexPageSize() - sizeof(uint32_t));
const uint32_t CachePage::CRC32_HEAD_OFFSET =
    (uint32_t)(Configure::GetDiskClusterSize() - sizeof(uint32_t));

CachePage::CachePage(IndexTree *indexTree, PageID pageId, PageType type)
    : _indexTree(indexTree), _pageId(pageId), _pageType(type),
      _fileId(indexTree->GetFileId()) {}
} // namespace storage
