#pragma once
#include "CachePage.h"

namespace storage {
class OverflowPage : public CachePage {
public:
  OverflowPage(IndexTree *indexTree, PageID startId, uint32_t pageNum)
      : CachePage(indexTree, startId, PageType::OVERFLOW_PAGE),
        _pageNum(pageNum) {
    _bysPage = CachePool::Apply(CACHE_PAGE_SIZE * pageNum);
  }

  ~OverflowPage() { CachePool::Release(_bysPage, CACHE_PAGE_SIZE * pageNum); }

  void ReadPage(PageFile *pageFile) override {
    PageFile *pFile =
        (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
    pFile->ReadPage(HRAD_PAGE_SIZE + _pageId * CACHE_PAGE_SIZE,
                    (char *)_bysPage, CACHE_PAGE_SIZE * _pageNum);

    if (pageFile == nullptr) {
      _indexTree->ReleasePageFile(pFile);
    }
  }
  void WritePage(PageFile *pageFile) override {}

protected:
  // How much pages for this overflow page
  uint32_t _pageNum;
};
} // namespace storage