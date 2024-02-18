#include "GarbageOwner.h"
#include "../pool/FilePagePool.h"
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
  FilePagePool::SyncReadPage(ovfPage);
  boost::crc_32_type crc32;
  crc32.process_bytes(ovfPage->GetBysPage(),
                      CachePage::INDEX_PAGE_SIZE * _usedPageNum);
  if (crc32.checksum() != c32) {

    LOG_ERROR << "Failed to verify garbage page. Index Name="
              << indexTree->GetFileName();
    abort();
  }

  for (uint32_t i = 0; i < items; i++) {
    PageID id = ovfPage->ReadInt(i * 6);
    uint16_t num = ovfPage->ReadShort(i * 6 + 4);
    InsertPage(id, num);
    _totalGarbagePages += num;
  }

  delete ovfPage;
};

/** Add new garbage pages into this class
 * PageId: the first page id
 * num: the pages number of this range
 */
void GarbageOwner::RecyclePage(PageID pid, uint16_t num, bool block) {
  struct {
    bool merge = false;
    PageID id;
    int16_t num;
  } l, r;
  unique_lock<ReentrantSpinMutex> lock(_spinMutex, std::defer_lock_t);
  if (block) {
    lock.lock();
  }
  _totalGarbagePages += num;
  _bDirty = true;
  if (_treeFreePage.size() == 0) {
    InsertPage(pid, num);
    return;
  }

  // To judge if it can merge right pages into a series pages.
  auto iter = _treeFreePage.upper_bound(pid);
  if (iter != _treeFreePage.end() && pid + num == iter->first) {
    r.merge = true;
    r.id = iter->first;
    r.num = iter->second;
  }

  // To judge if it can merge left pages into a series pages.
  if (iter != _treeFreePage.begin()) {
    iter--;
    if (iter->first + iter->second == pid) {
      l.merge = true;
      l.id = iter->first;
      l.num = iter->second;
    }
  }
  // If true, merge right pages into a series pages.
  if (l.merge && (int32_t)num + l.num < UINT16_MAX) {
    ErasePage(l.id, l.num);
    pid = l.id;
    num = l.num + num;
  }
  // If true, merge left pages into a series pages.
  if (r.merge && (int32_t)num + r.num < UINT16_MAX) {
    ErasePage(r.id, r.num);
    num += r.num;
  }

  InsertPage(pid, num);
}

/** @brief Apply a series of page ids for a overflow page.
 * @param num: input the expected page number
 * @param block: if lock spin mutex
 * @return: the first page id
 */
PageID GarbageOwner::ApplyOvfPage(uint16_t num, bool block) {
  if (_totalGarbagePages < num) {
    return PAGE_NULL_POINTER;
  }
  unique_lock<SpinMutex> lock(_spinMutex, defer_lock);
  if (block && !lock.try_lock()) {
    return PAGE_NULL_POINTER;
  }

  auto iter = _rangePage.lower_bound(num);
  if (iter == _rangePage.end()) {
    return PAGE_NULL_POINTER;
  }

  uint16_t num2 = iter->first;
  PageID id = *iter->second.begin();
  ErasePage(id, num2);

  if (num2 > num) {
    InsertPage(id + num, num2 - num);
  }
  _totalGarbagePages -= num;
  _bDirty = true;
  return id;
}

/** @brief Apply multi index pages' ids
 * @param num: input the expected index page number
 * @param block: if lock spin mutex
 * @return: the vector of index page ids
 */
vector<PageID> GarbageOwner::ApplyIndexPages(uint16_t num, bool block) {
  if (_totalGarbagePages == 0) {
    return {};
  }
  unique_lock<SpinMutex> lock(_spinMutex, defer_lock);
  if (block && !lock.try_lock()) {
    return PAGE_NULL_POINTER;
  }
  vector<PageID> vct;
  for (uint16_t i = 0; i < num;) {
    auto iter = _rangePage.lower_bound(num);
    if (iter == _rangePage.end()) {
      break;
    }

    uint16_t num2 = iter->first;
    PageID id = *iter->second.begin();
    for (uint16_t j = 0; j < num2 && i < num; j++; i++) {
      vct.push_back(id + j);
    }

    ErasePage(id, num2);
    if (num2 > num) {
      InsertPage(id + num, num2 - num);
    }
  }

  _totalGarbagePages -= num;
  _bDirty = true;
  return id;
}

/**Every check point time will call this to save garbage page ids into disk*/
bool GarbageOwner::SavePage(bool block) {
  if (_ovfPage != nullptr && _ovfPage->GetPageStatus() != PageStatus::VALID) {
    return false;
  }

  unique_lock<SpinMutex> lock(_spinMutex, defer_lock);
  if (block) {
    lock.lock();
  }

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
  // Every 6 bytes to save the first page id and range size
  _usedPageNum =
      (uint16_t)((_treeFreePage.size() * 6 + CachePage::INDEX_PAGE_SIZE - 1) /
                 CachePage::INDEX_PAGE_SIZE);
  _firstPageId = ApplyPage(_usedPageNum, false);
  if (_firstPageId == PAGE_NULL_POINTER) {
    _firstPageId =
        _indexTree->GetHeadPage()->GetAndIncTotalPageCount(_usedPageNum, block);
  }

  if (_ovfPage != nullptr) {
    delete _ovfPage;
  }

  _ovfPage = new OverflowPage(_indexTree, _firstPageId, _usedPageNum, true);
  int count = 0;
  for (auto iter = _treeFreePage.begin(); iter != _treeFreePage.end(); iter++) {
    ovfPage->WriteInt(count, iter->first);
    ovfPage->WriteShort(count + 4, iter->second);
    count += 6;
  }

  boost::crc_32_type crc32;
  uint32_t c32 = crc32.process_bytes(ovfPage->GetBysPage(),
                                     CachePage::INDEX_PAGE_SIZE * _usedPageNum);

  _indexTree->GetHeadPage()->WriteGabage(_totalGarbagePages, _firstPageId,
                                         _usedPageNum, c32);
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
