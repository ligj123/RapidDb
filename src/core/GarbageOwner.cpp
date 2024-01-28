#include "GarbageOwner.h"
#include "../pool/StoragePool.h"
#include "../utils/Log.h"
#include "CachePage.h"
#include "IndexTree.h"
#include <boost/crc.hpp>

namespace storage {
GarbageOwner::GarbageOwner(IndexTree *indexTree)
    : _indexTree(indexTree), _bDirty(false) {
  HeadPage *headPage = indexTree->GetHeadPage();

  _totalGarbagePages = 0;
  uint32_t c32;
  uint32_t items;
  headPage->ReadGarbage(items, _firstPageId, _usedPageNum, c32);
  if (_usedPageNum == 0)
    return;
  OverflowPage *ovfPage =
      new OverflowPage(indexTree, _firstPageId, _usedPageNum, false);
  ovfPage->ReadPage(nullptr);
  boost::crc_32_type crc32;
  crc32.process_bytes(ovfPage->GetBysPage(),
                      CachePage::INDEX_PAGE_SIZE * _usedPageNum);
  if (crc32.checksum() != c32) {

    LOG_ERROR << "Failed to verify garbage page. Index Name="
              << indexTree->GetFileName();
    return;
  }

  for (uint32_t i = 0; i < items; i++) {
    PageID id = ovfPage->ReadInt(i * 6);
    uint16_t num = ovfPage->ReadShort(i * 6 + 4);
    InsertPage(id, num);
    _totalGarbagePages += num;
  }

  ovfPage->DecRef();
};

/** Add new garbage pages into this class
 * PageId: the first page id
 * num: the pages number of this range
 */
void GarbageOwner::RecyclePage(PageID pid, uint16_t num) {
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
  _bDirty = true;
}

/** Apply a series of pages.
 * num: input the expected page number
 * return: the first page id
 */
PageID GarbageOwner::ApplyPage(uint16_t num) {
  if (_totalGarbagePages < num || _rangePage.rbegin()->first < num) {
    return PAGE_NULL_POINTER;
  }
  unique_lock<ReentrantSpinMutex> lock(_spinMutex, defer_lock);
  if (!lock.try_lock()) {
    return PAGE_NULL_POINTER;
  }
  auto iter = _rangePage.lower_bound(num);
  if (iter == _rangePage.end()) {
    return PAGE_NULL_POINTER;
  }

  int16_t num2 = iter->first;
  PageID id = *iter->second.begin();
  ErasePage(id, num2);

  if (num2 > num) {
    InsertPage(id + num, num2 - num);
  }
  _totalGarbagePages -= num;
  _bDirty = true;
  return id;
}
/**Every check point time will call this to save garbage page ids into disk*/
void GarbageOwner::SavePage() {
  unique_lock<ReentrantSpinMutex> lock(_spinMutex);
  _bDirty = false;
  if (_usedPageNum > 0) {
    ReleasePage(_firstPageId, _usedPageNum);
  }

  if (_totalGarbagePages == 0) {
    _firstPageId = PAGE_NULL_POINTER;
    _usedPageNum = 0;
    _indexTree->GetHeadPage()->WriteGabage(_totalGarbagePages, _firstPageId,
                                           _usedPageNum, 0);
    return;
  }
  // Every 6 bytes to save the first page id and range size, the last 4 bytes
  // save crc32 code for verify
  _usedPageNum =
      (uint16_t)((_treeFreePage.size() * 6 + CachePage::INDEX_PAGE_SIZE - 1) /
                 CachePage::INDEX_PAGE_SIZE);
  _firstPageId = ApplyPage(_usedPageNum);
  if (_firstPageId == PAGE_NULL_POINTER) {
    _firstPageId =
        _indexTree->GetHeadPage()->GetAndIncTotalPageCount(_usedPageNum);
  }

  OverflowPage *ovfPage =
      new OverflowPage(_indexTree, _firstPageId, _usedPageNum, true);
  int count = 0;
  for (auto iter = _treeFreePage.begin(); iter != _treeFreePage.end(); iter++) {
    ovfPage->WriteInt(count, iter->first);
    ovfPage->WriteShort(count + 4, iter->second);
    count += 6;
  }

  ovfPage->WritePage(nullptr);
  ovfPage->DecRef(2);
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
