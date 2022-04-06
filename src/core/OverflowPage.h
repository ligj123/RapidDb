#pragma once
#include "CachePage.h"

namespace storage {
class OverflowPage : public CachePage {
public:
  OverflowPage(IndexTree *indexTree, PageID pid)
      : CachePage(indexTree, pid, PageType::OVERFLOW_PAGE) {}

protected:
};
} // namespace storage