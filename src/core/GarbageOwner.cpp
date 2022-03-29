#include "GarbageOwner.h"
#include "CachePage.h"

namespace storage {
const uint16_t GarbageOwner::PAGE_TYPE_OFFSET = 0;

GarbageOwner::GarbageOwner(IndexTree *indexTree) : _indexTree(indexTree) {
  HeadPage *headPage = indexTree->GetHeadPage();
  headPage->ReadGarbage(_totalGarbagePages, _firstPageId, _usedPageNum);
  if (_usedPageNum == 0)
    return;

  uint32_t sz = (uint32_t)Configure::GetDiskClusterSize() - 10;
  int count = 0;
  for (int32_t i = 0; i < _usedPageNum && count < _totalGarbagePages; i++) {
    CachePage *page = new CachePage(indexTree, _firstPageId + i);
    page->ReadPage();

    for (int32_t i = 0; i < sz && count < _totalGarbagePages; i += 6) {
      PageID id = page->ReadInt(i);
      uint16_t num = page->ReadShort(i + 4);
      InsertPage(id, num);
      count += num;
    }

    delete page;
  }
};

/** Add new garbage pages into this class
 * PageId: the first page id
 * num: the pages number of this range
 */
void GarbageOwner::ReleasePage(PageID pid, uint16_t num) {
  struct {
    bool merge = false;
    PageID id;
    int16_t num;
  } l, r;
  unique_lock<ReentrantSpinMutex> lock(_spinMutex);
  if (_treeFreePage.size() == 0) {
    InsertPage(pid, num);
    return;
  }

  auto iter = _treeFreePage.upper_bound(pid);
  if (iter != _treeFreePage.end() && pid + num == iter->first) {
    r.merge = true;
    r.id = iter->first;
    r.num = iter->second;
  }

  iter--;
  if (iter != _treeFreePage.end() && iter->first + iter->second == pid) {
    l.merge = true;
    l.id = iter->first;
    l.num = iter->second;
  }

  if (l.merge && (int)num + l.num < UINT16_MAX) {
    ErasePage(l.id, l.num);
    pid = l.id;
    num = l.num + num;
  }

  if (r.merge && (int)num + r.num < UINT16_MAX) {
    ErasePage(r.id, r.num);
    num += r.num;
  }

  InsertPage(pid, num);
  _totalGarbagePages += num;
}

/** Apply a series of pages.
 * num: input the expected page number
 * return: the first page id
 */
PageID GarbageOwner::ApplyPage(uint16_t num) {
  if (_totalGarbagePages < num || _rangePage.rbegin()->first < num) {
    return HeadPage::PAGE_NULL_POINTER;
  }
  unique_lock<ReentrantSpinMutex> lock(_spinMutex, defer_lock);
  if (!lock.try_lock()) {
    return HeadPage::PAGE_NULL_POINTER;
  }
  auto iter = _rangePage.lower_bound(num);
  if (iter == _rangePage.end()) {
    return HeadPage::PAGE_NULL_POINTER;
  }

  int16_t num2 = iter->first;
  PageID id = *iter->second.begin();
  ErasePage(id, num2);

  if (num2 > num) {
    InsertPage(id + num, num2 - num);
  }
  _totalGarbagePages -= num;
  return id;
}
/**Every check point time will call this to save garbage page ids into disk*/
void GarbageOwner::SavePage() {
  unique_lock<ReentrantSpinMutex> lock(_spinMutex);
  if (_usedPageNum > 0) {
    ReleasePage(_firstPageId, _usedPageNum);
    _totalGarbagePages += _usedPageNum;
  }

  if (_totalGarbagePages == 0) {
    _firstPageId = HeadPage::PAGE_NULL_POINTER;
    _usedPageNum = 0;
    _indexTree->GetHeadPage()->WriteGabage(_totalGarbagePages, _firstPageId,
                                           _usedPageNum);
    return;
  }
  // Every 4 bytes to save one page id, the last 4 bytes save crc32 code for
  // verify
  int pNum = (Configure::GetCachePageSize() - 4) / 6;
  _usedPageNum = (uint16_t)((_treeFreePage.size() + pNum - 1) / pNum);
  _firstPageId = ApplyPage(_usedPageNum);
  if (_firstPageId == HeadPage::PAGE_NULL_POINTER) {
    _firstPageId =
        _indexTree->GetHeadPage()->GetAndIncTotalPageCount(_usedPageNum);
  }

  CachePage *cpage = new CachePage(_indexTree, _firstPageId);
  for (auto iter = _treeFreePage.begin(); iter != _treeFreePage.end(); iter++) {
  }
}

void GarbageOwner::InsertPage(PageID pageId, int16_t num) {
  _treeFreePage.insert({pageId, num});
  auto iter = _rangePage.try_emplace(num);
  iter.first->second.insert(pageId);
}
void GarbageOwner::ErasePage(PageID pageId, int16_t num) {
  _treeFreePage.erase(pageId);
  auto iter = _rangePage.find(num);
  iter->second.erase(pageId);
  if (iter->second.size() == 0) {
    _rangePage.erase(iter);
  }
}
} // namespace storage