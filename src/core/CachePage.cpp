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

// void CachePage::AfterRead() {
//   bool bvalid = true;
//   crc32.reset();
//   if (_pageType == PageType::HEAD_PAGE) {
//     crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
//     if (crc32.checksum() != (uint32_t)ReadInt(CRC32_HEAD_OFFSET)) {
//       bvalid = false;
//     }
//   } else if (_pageType != PageType::OVERFLOW_PAGE) {
//     crc32.process_bytes(_bysPage, CRC32_INDEX_OFFSET);
//     if (crc32.checksum() != (uint32_t)ReadInt(CRC32_INDEX_OFFSET)) {
//       bvalid = false;
//     }
//   }

//   if (bvalid) {
//     _bDirty = false;
//     Init();
//     _pageStatus = PageStatus::VALID;
//   } else {
//     _pageStatus = PageStatus::INVALID;
//     // Now if cache page is invalid, it will abort; In following version, it
//     // will add the function to fix the invalid page
//     abort();
//   }
// }

// void CachePage::SaveCrc32() {
//   boost::crc_32_type crc32;
//   if (_pageType == PageType::HEAD_PAGE) {
//     crc32.process_bytes(_bysPage, CRC32_HEAD_OFFSET);
//     WriteInt(CRC32_HEAD_OFFSET, crc32.checksum());
//   } else if (_pageType != PageType::OVERFLOW_PAGE) {
//     crc32.process_bytes(_bysPage, CRC32_INDEX_OFFSET);
//     WriteInt(CRC32_INDEX_OFFSET, crc32.checksum());
//   }
// }
} // namespace storage
