#include "OverflowPage.h"
#include "../pool/PageBufferPool.h"
#include "IndexTree.h"

namespace storage {
OverflowPage *OverflowPage::GetPage(IndexTree *indexTree, PageID startId,
                                    uint16_t pageNum, bool bNew) {
  if (!bNew) {
    OverflowPage *ovp = (OverflowPage *)PageBufferPool::GetPage(
        indexTree->GetFileId(), startId);
    if (ovp != nullptr)
      return ovp;
  }

  OverflowPage *ovp = new OverflowPage(indexTree, startId, pageNum);
  PageBufferPool::AddPage(ovp);
  return ovp;
}

void OverflowPage::ReadPage(PageFile *pageFile) {
  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  pFile->ReadPage(HEAD_PAGE_SIZE + _pageId * CACHE_PAGE_SIZE, (char *)_bysPage,
                  CACHE_PAGE_SIZE * _pageNum);

  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
  _bFilled = true;
}

void OverflowPage::WritePage(PageFile *pageFile) {
  PageFile *pFile =
      (pageFile == nullptr ? _indexTree->ApplyPageFile() : pageFile);
  pFile->WritePage(HEAD_PAGE_SIZE + _pageId * CACHE_PAGE_SIZE, (char *)_bysPage,
                   CACHE_PAGE_SIZE * _pageNum);

  if (pageFile == nullptr) {
    _indexTree->ReleasePageFile(pFile);
  }
}
} // namespace storage
