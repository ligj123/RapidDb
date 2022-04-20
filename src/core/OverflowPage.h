#pragma once
#include "CachePage.h"

namespace storage {
class OverflowPage : public CachePage {
public:
  OverflowPage(IndexTree *indexTree, PageID startId, uint16_t pageNum)
      : CachePage(indexTree, startId, PageType::OVERFLOW_PAGE),
        _pageNum(pageNum) {
    _bysPage = CachePool::Apply(CACHE_PAGE_SIZE * pageNum);
  }

  ~OverflowPage() { CachePool::Release(_bysPage, CACHE_PAGE_SIZE * _pageNum); }

  void ReadPage(PageFile *pageFile) override;
  void WritePage(PageFile *pageFile) override;
  uint16_t GetPageNum() const { return _pageNum; }

protected:
  // How much pages for this overflow page
  uint16_t _pageNum;
};
} // namespace storage
