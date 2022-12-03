#pragma once
#include "CachePage.h"

namespace storage {
class OverflowPage : public CachePage {
public:
  static OverflowPage *GetPage(IndexTree *indexTree, PageID startId,
                               uint16_t pageNum, bool bNew);

public:
  OverflowPage(IndexTree *indexTree, PageID startId, uint16_t pageNum,
               bool bNew)
      : CachePage(indexTree, startId, PageType::OVERFLOW_PAGE),
        _pageNum(pageNum) {
    _bysPage = CachePool::Apply(CACHE_PAGE_SIZE * pageNum);
    if (bNew) {
      _pageStatus == PageStatus::VALID;
    }
  }
  void ReadPage(PageFile *pageFile) override;
  void WritePage(PageFile *pageFile) override;
  uint16_t GetPageNum() const { return _pageNum; }

protected:
  ~OverflowPage() { CachePool::Release(_bysPage, CACHE_PAGE_SIZE * _pageNum); }
  // How much pages for this overflow page
  uint16_t _pageNum;
};
} // namespace storage
