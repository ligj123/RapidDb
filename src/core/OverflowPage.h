#pragma once
#include "CachePage.h"

namespace storage {
class OverflowPage : public CachePage {
public:
  static OverflowPage *GetPage(IndexTree *indexTree, PageID startId,
                               uint16_t pageNum, bool bNew = false);

public:
  OverflowPage(IndexTree *indexTree, PageID startId, uint16_t pageNum,
               bool filled)
      : CachePage(indexTree, startId, PageType::OVERFLOW_PAGE),
        _pageNum(pageNum), _bFilled(filled) {
    _bysPage = CachePool::Apply(CACHE_PAGE_SIZE * pageNum);
  }
  void ReadPage(PageFile *pageFile) override;
  void WritePage(PageFile *pageFile) override;
  uint16_t GetPageNum() const { return _pageNum; }
  bool IsFilled() { return _bFilled; }

protected:
  ~OverflowPage() { CachePool::Release(_bysPage, CACHE_PAGE_SIZE * _pageNum); }
  // How much pages for this overflow page
  uint16_t _pageNum;
  // If this page has been filled value or loaded from disk
  bool _bFilled;
};
} // namespace storage
