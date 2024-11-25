#include "CachePagePool.h"
#include "../core/BranchPage.h"
#include "../core/LeafPage.h"
#include "../utils/Log.h"

#include <forward_list>
#include <shared_mutex>

namespace storage {
uint64_t CachePagePool::_maxCacheSize =
    Configure::GetTotalMemorySize() / Configure::GetIndexPageSize();
unordered_map<uint64_t, CachePage *>
    CachePagePool::_mapCache(CachePagePool::_maxCacheSize);
SpinMutex CachePagePool::_spinMutex;
atomic_bool CachePagePool::_urgentTask{false};
thread *CachePagePool::_thread{nullptr};
bool CachePagePool::_bStoped{false};
uint64_t CachePagePool::_countPool{0};

void CachePagePool::AddPage(CachePage *page) {
  unique_lock<SpinMutex> lock(_spinMutex);
  _mapCache.emplace(page->HashCode(), page);
}

void CachePagePool::AddPages(MVector<IndexPage *> &vctPage) {
  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto page : vctPage) {
    _mapCache.emplace(page->HashCode(), page);
  }
}

CachePage *CachePagePool::GetPage(IndexTree *idxTree, uint32_t pageId,
                                  PageType type) {
  unique_lock<SpinMutex> lock(_spinMutex);
  uint64_t hashId = CachePage::CalcHashCode(idxTree->GetFileId(), pageId);
  auto iter = _mapCache.find(hashId);
  if (iter == _mapCache.end()) {
    IndexPage *page;
    if (type == PageType::LEAF_PAGE) {
      page = new LeafPage(idxTree, pageId);
    } else {
      assert(type == PageType::BRANCH_PAGE);
      page = new BranchPage(idxTree, pageId);
    }

    idxTree->IncPages();
    page->SetReferred(true);
    _mapCache.emplace(hashId, page);
    return page;
  } else {
    iter->second->SetReferred(true);
    return iter->second;
  }
}

void CachePagePool::InitPool() {
  assert(_thread == nullptr);
  _thread = new thread([]() {
    while (!_bStoped) {
      this_thread::sleep_for(1us);
      PoolManage();
    }
  });
}

void CachePagePool::StopPool() {
  assert(_thread != nullptr);
  _bStoped = true;
  _thread->join();
  _thread = nullptr;

  ClearPool();
}

void CachePagePool::ClearPool() {
  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto iter = _mapCache.begin(); iter != _mapCache.end(); iter++) {
    CachePage *page = iter->second;
    assert(!page->IsRefered() && !page->IsDirty() &&
           page->GetPageStatus() == PageStatus::VALID);
    delete page;
  }

  _mapCache.clear();
}

void CachePagePool::PoolManage() {
  _countPool++;
  bool pass = false;
  if (_urgentTask.load(memory_order_relaxed)) {
    _urgentTask.store(false, memory_order_relaxed);
    pass = true;
  } else if (_countPool % 100000 != 0) {
    return;
  }

  MemoryStatus status = CachePool::GetMemoryStatus();
  if (!pass &&
      (status == MemoryStatus::FATAL ||
       (status == MemoryStatus::CRITICAL && _countPool % 1000000 == 0) ||
       _countPool % 10000000 == 0)) {
    pass = true;
  }
  if (!pass)
    return;

  // The min score that a page can exist in CachePagePool
  uint32_t min_score;
  switch (status) {
  case MemoryStatus::AMPLE:
    min_score = 1000;
    break;
  case MemoryStatus::SCARE:
    min_score = 100000;
    break;
  case MemoryStatus::CRITICAL:
    min_score = 10000000;
    break;
  case MemoryStatus::FATAL:
  default:
    min_score = UINT32_MAX;
    break;
  }

  int delCount = 0;
  forward_list<CachePage *> flist;

  for (auto iter = _mapCache.begin(); iter != _mapCache.end(); iter++) {
    CachePage *page = iter->second;
    uint32_t score;
    if (_countPool % 10000000) {
      score = page->GetScore();
    } else {
      score = page->CalcScore();
    };

    if (!page->Releaseable()) {
      continue;
    }

    if (!page->GetIndexTree()->IsClosed() && score > min_score) {
      continue;
    }

    flist.push_front(page);
  }

  if (flist.empty())
    return;

  {
    unique_lock<SpinMutex> lock(_spinMutex);

    for (CachePage *page : flist) {
      page->GetIndexTree()->DecPages();
      _mapCache.erase(page->HashCode());
    }
  }

  LOG_INFO << "MaxPage=" << _maxCacheSize << "\tUsedPage=" << _mapCache.size()
           << "\tRemoved page:" << delCount;
}

} // namespace storage
