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
    _bysPage = CachePool::Apply(INDEX_PAGE_SIZE * pageNum);
    if (bNew) {
      _pageStatus = PageStatus::VALID;
    } else {
      _pageStatus = PageStatus::READING;
      // TO DO (Add into FilePagePool)
    }
  }
  ~OverflowPage() { CachePool::Release(_bysPage, INDEX_PAGE_SIZE * _pageNum); }
  void AfterRead() override { _pageStatus = PageStatus::READED; }
  void AfterWrite() override { _pageStatus = PageStatus::VALID; }
  uint16_t GetPageNum() const { return _pageNum; }
  uint32_t PageSize() const override { return INDEX_PAGE_SIZE * _pageNum; }

protected:
  // How much pages for this overflow page
  uint16_t _pageNum;
};
} // namespace storage
