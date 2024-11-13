#include "IndexPage.h"
#include "../binlog/LogRecord.h"
#include "../binlog/LogServer.h"
#include "BranchPage.h"
#include "BranchRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"

namespace storage {
const uint16_t IndexPage::LOAD_FACTOR = 90;
const uint32_t IndexPage::LOAD_THRESHOLD = CachePage::INDEX_PAGE_SIZE * 3;
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
} // namespace storage
