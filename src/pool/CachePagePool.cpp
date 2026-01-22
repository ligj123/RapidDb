#include "CachePagePool.h"
#include "../core/BranchPage.h"
#include "../core/LeafPage.h"
#include "../utils/Log.h"

#include <forward_list>
#include <shared_mutex>

namespace storage {
uint64_t CachePagePool::_maxCacheSize =
    Configure::GetTotalMemorySize() / Configure::GetIndexPageSize();
unordered_map<uint64_t, IndexPage *>
    CachePagePool::_mapCache(CachePagePool::_maxCacheSize);
SpinMutex CachePagePool::_spinMutex;
uint64_t CachePagePool::_midPage{UINT64_MAX};
CachePagePoolTask *CachePagePoolTask::_instance{nullptr};
MTreeMap<uint64_t, IndexPage *> CachePagePool::_tmpCache;

void CachePagePoolTask::Init(ThreadPool *tpool) {
  _instance = new CachePagePoolTask();
  _instance->_dtLastVisit = MicroSecTime();
  tpool->AddTask(_instance);
}

TaskStatus CachePagePoolTask::Run() {
  DT_MicroSec dt = MicroSecTime();
  DT_MicroSec span = dt - _dtLastVisit;

  MemoryStatus status = CachePool::GetMemoryStatus();
  if (!_bStoped && span > 1000 + 1000 * pow(4, 4 - (int)status)) {
    return TaskStatus::INTERVAL;
  }
  _taskStatus.store(TaskStatus::RUNNING, memory_order_relaxed);
  CachePagePool::PoolManage(status);
  if (_bStoped && CachePagePool::_mapCache.size() == 0 &&
      CachePagePool::_tmpCache.size() == 0) {
    _taskStatus.store(TaskStatus::FINISHED, memory_order_relaxed);
    return TaskStatus::FINISHED;
  } else {
    _taskStatus.store(TaskStatus::INTERVAL, memory_order_relaxed);
    return TaskStatus::INTERVAL;
  }
}

void CachePagePool::AddPage(IndexPage *page) {
  unique_lock<SpinMutex> lock(_spinMutex);
  assert(_mapCache.find(page->HashCode()) == _mapCache.end());
  auto res = _tmpCache.emplace(page->HashCode(), page);
  assert(res.second); // To ensure the page is new.
}

void CachePagePool::AddPages(MVector<IndexPage *> &vctPage) {
  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto page : vctPage) {
    assert(_mapCache.find(page->HashCode()) == _mapCache.end());
    auto res = _tmpCache.emplace(page->HashCode(), page);
    assert(res.second); // To ensure the page is new.
  }
}

IndexPage *CachePagePool::GetPage(IndexTree *idxTree, PageID pageId,
                                  PageType type) {
  unique_lock<SpinMutex> lock(_spinMutex);
  uint64_t hashId = CachePage::CalcHashCode(idxTree->GetFileId(), pageId);
  auto iter = _mapCache.find(hashId);
  if (iter == _mapCache.end()) {
    auto iter2 = _tmpCache.find(hashId);
    if (iter2 != _tmpCache.end()) {
      iter2->second->SetReferred(true);
      return iter2->second;
    }

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

MVector<IndexPage *> CachePagePool::GetPages(IndexTree *idxTree, PageType type,
                                             MVector<PageID> &vctPageId) {
  unique_lock<SpinMutex> lock(_spinMutex);
  assert(vctPageId.size() > 0);
  MVector<IndexPage *> vctPage;
  vctPage.reserve(vctPageId.size());

  for (PageID pid : vctPageId) {
    uint64_t hashId = CachePage::CalcHashCode(idxTree->GetFileId(), pid);
    auto iter = _mapCache.find(hashId);
    if (iter == _mapCache.end()) {
      auto iter2 = _tmpCache.find(hashId);
      if (iter2 != _tmpCache.end()) {
        iter2->second->SetReferred(true);
        vctPage.push_back(iter2->second);
        continue;
      }

      IndexPage *page;
      if (type == PageType::LEAF_PAGE) {
        page = new LeafPage(idxTree, pid);
      } else {
        assert(type == PageType::BRANCH_PAGE);
        page = new BranchPage(idxTree, pid);
      }

      idxTree->IncPages();
      page->SetReferred(true);
      _mapCache.emplace(hashId, page);
      vctPage.push_back(page);
    } else {
      iter->second->SetReferred(true);
      vctPage.push_back(iter->second);
    }
  }

  return vctPage;
}

void CachePagePool::ClearPool() {
  unique_lock<SpinMutex> lock(_spinMutex);
  for (auto iter = _mapCache.begin(); iter != _mapCache.end(); iter++) {
    IndexPage *page = iter->second;
    assert(!page->IsRefered());
    delete page;
  }

  for (auto iter = _tmpCache.begin(); iter != _tmpCache.end(); iter++) {
    IndexPage *page = iter->second;
    assert(!page->IsRefered());
    delete page;
  }

  _mapCache.clear();
  _tmpCache.clear();
}

void CachePagePool::PoolManage(MemoryStatus status) {
  // The min score that a page can exist in CachePagePool
  uint32_t min_score;
  switch (status) {
  case MemoryStatus::AMPLE:
    min_score = 30;
    break;
  case MemoryStatus::SCARE:
    min_score = 1000;
    break;
  case MemoryStatus::CRITICAL:
    min_score = 30000;
    break;
  case MemoryStatus::FATAL:
  default:
    min_score = UINT16_MAX;
    break;
  }

  MList<unordered_map<uint64_t, IndexPage *>::iterator> lst;
  auto iter =
      (_midPage == UINT64_MAX ? _mapCache.begin() : _mapCache.find(_midPage));

  for (; iter != _mapCache.end() && lst.size() < 100000; iter++) {
    CachePage *page = iter->second;
    uint32_t score = page->CalcScore();

    if (!page->Releaseable()) {
      continue;
    }

    if (!page->GetIndexTree()->IsClosed() && score > min_score) {
      continue;
    }

    lst.push_back(iter);
  }

  if (iter == _mapCache.end()) {
    _midPage = iter->first;
  } else {
    _midPage = UINT64_MAX;
  }

  if (lst.empty() && _tmpCache.size() == 0)
    return;

  unique_lock<SpinMutex> lock(_spinMutex);

  for (auto iter : lst) {
    iter->second->GetIndexTree()->DecPages();
    delete iter->second;
    _mapCache.erase(iter);
  }

  for (auto iter = _tmpCache.begin(); iter != _tmpCache.end(); iter++) {
    auto iter2 = _mapCache.emplace(iter->second->HashCode(), iter->second);
    assert(iter2.second);
  }

  _tmpCache.clear();
  lock.unlock();

  LOG_INFO << "MaxPage=" << _maxCacheSize << "\tUsedPage=" << _mapCache.size()
           << "\tRemoved page:" << lst.size();
}

} // namespace storage
