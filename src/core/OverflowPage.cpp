#include "OverflowPage.h"
#include "../pool/PageBufferPool.h"
#include "IndexTree.h"

namespace storage {
OverflowPage *OverflowPage::GetPage(IndexTree *indexTree, PageID startId,
                                    uint16_t pageNum, bool bNew) {
  OverflowPage *ovp = new OverflowPage(indexTree, startId, pageNum, bNew);
  return ovp;
}

} // namespace storage
