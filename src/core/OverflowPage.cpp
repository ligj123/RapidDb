#pragma once
#include "OverflowPage.h"
#include "IndexTree.h"

namespace storage {
void OverflowPage::ReadPage(PageFile *pageFile) {
  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  pFile->ReadPage(HEAD_PAGE_SIZE + _pageId * CACHE_PAGE_SIZE, (char *)_bysPage,
                  CACHE_PAGE_SIZE * _pageNum);

  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
}

void OverflowPage::WritePage(PageFile *pageFile) {
  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  char *tmp = PageFile::_ovfBuff.GetBuf();

  pFile->WritePage(HEAD_PAGE_SIZE + _pageId * CACHE_PAGE_SIZE, (char *)_bysPage,
                   CACHE_PAGE_SIZE * _pageNum);

  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
}
} // namespace storage
