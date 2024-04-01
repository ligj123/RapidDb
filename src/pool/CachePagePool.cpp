#include "CachePagePool.h"
#include "../core/IndexTree.h"
#include "../utils/Log.h"
#include "PageDividePool.h"
#include "StoragePool.h"
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
  page->GetIndexTree()->IncPages();
  _mapCache.emplace(page->HashCode(), page);
}

CachePage *CachePagePool::GetPage(uint64_t hashId) {
  unique_lock<SpinMutex> lock(_spinMutex);
  auto iter = _mapCache.find(hashId);
  if (iter == _mapCache.end())
    return nullptr;
  else {
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
    iter->second->DecPage();
    delete iter->second;
  }
}

void CachePagePool::PoolManage() {
  _countPool++;
  bool pass = false;
  if (_urgentTask.load(memory_order_relaxed)) {
    _urgentTask.store(false, memory_order_relaxed);
    pass = true;
  } else if (count % 100000 != 0) {
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

  LOG_INFO << "MaxPage=" << _maxCacheSize << "\tUsedPage=" << _mapCache.Size()
           << "\tRemoved page:" << delCount;
}

} // namespace storage
