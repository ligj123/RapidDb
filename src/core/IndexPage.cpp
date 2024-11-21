#include "IndexPage.h"
#include "../binlog/LogRecord.h"
#include "../binlog/LogServer.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
const uint16_t IndexPage::LOAD_FACTOR = 90;
const uint32_t IndexPage::LOAD_THRESHOLD = CachePage::INDEX_PAGE_SIZE * 5;
const uint16_t IndexPage::PAGE_LEVEL_OFFSET = 0;
const uint16_t IndexPage::PAGE_BEGIN_END_OFFSET = 1;
const uint16_t IndexPage::PAGE_TRAN_COUNT_OFFSET = 2;
const uint16_t IndexPage::NUM_RECORD_OFFSET = 4;
const uint16_t IndexPage::TOTAL_DATA_LENGTH_OFFSET = 6;
const uint16_t IndexPage::PARENT_PAGE_POINTER_OFFSET = 8;

IndexPage::~IndexPage() {
  _indexTree->DecPages(1);
  CachePool::ReleasePage(_bysPage);
}
void IndexPage::AfterRead() {
  boost::crc_32_type crc32;
  crc32.process_bytes(_bysPage, CRC32_INDEX_OFFSET);
  if (crc32.checksum() != (uint32_t)ReadInt(CRC32_INDEX_OFFSET)) {
    _pageStatus.store(PageStatus::INVALID, memory_order_relaxed);
    // TO DO
    // Now if cache page is invalid, it will abort; In following version, it
    // will add the function to fix the invalid page
    abort();
  } else {
    InitParameters();
    _pageStatus.store(PageStatus::READED, memory_order_release);
  }
}
} // namespace storage
