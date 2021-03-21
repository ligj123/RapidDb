#include "PageBufferPool.h"
#include <shared_mutex>
#include "../core/IndexTree.h"
#include "../utils/Log.h"
#include "PageDividePool.h"
#include "StoragePool.h"

namespace storage {
  unordered_map<uint64_t, IndexPage*> PageBufferPool::_mapCache;
  utils::SharedSpinMutex PageBufferPool::_rwLock;
  int64_t PageBufferPool::_maxCacheSize = Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
  thread PageBufferPool::_tIndexPageManager;
  int64_t PageBufferPool::_prevDelNum = 100;
  thread* PageBufferPool::_pageBufferThread = PageBufferPool::CreateThread();
  bool PageBufferPool::_bSuspend = false;

  void PageBufferPool::AddPage(IndexPage* page) {
    unique_lock<utils::SharedSpinMutex> lock(_rwLock);
    _mapCache.insert(pair<uint64_t, IndexPage*>(page->HashCode(), page));
  }

  IndexPage* PageBufferPool::GetPage(uint64_t pageId) {
    shared_lock<utils::SharedSpinMutex> lock(_rwLock);
    auto iter = _mapCache.find(pageId);
    if (iter != _mapCache.end()) {
      iter->second->UpdateAccessTime();
      iter->second->IncRefCount();
      return iter->second;
    }

    return nullptr;
  }

  void PageBufferPool::ClearPool() {
    unique_lock<utils::SharedSpinMutex> lock(_rwLock);
    for (auto iter = _mapCache.begin(); iter != _mapCache.end(); iter++) {
      IndexPage* page = iter->second;
      while (!page->Releaseable()) {
        this_thread::sleep_for(std::chrono::microseconds(10));
      }

      page->GetIndexTree()->DecPages();
      delete page;
    }
    _mapCache.clear();
  }

  thread* PageBufferPool::CreateThread() {
    thread* t = new thread([]() {
      while (true) {
        this_thread::sleep_for(std::chrono::milliseconds(1000 * 5));
        if (_bSuspend) continue;
        try {
          PoolManage();
        }
        catch (...) {
          LOG_ERROR << "Catch unknown exception!";
        }
      }
      });
    return t;
  }

  void PageBufferPool::PoolManage() {
    int64_t numDel = _mapCache.size() - _maxCacheSize;
    if (numDel <= 0) {
      return;
    }

    if (numDel > _prevDelNum * 2) {
      numDel = _prevDelNum * 2;
      if (numDel > 100000)
        numDel = 100000;
    }
    else if (numDel < _prevDelNum / 2)
    {
      numDel = _prevDelNum / 2;
      if (numDel < 500)
        numDel = 500;
    }

    _prevDelNum = numDel;
    _rwLock.lock_shared();
    static auto cmp = [](IndexPage* left, IndexPage* right) { return left->GetAccessTime() < right->GetAccessTime(); };
    priority_queue<IndexPage*, vector<IndexPage*>, decltype(cmp)> queue(cmp);

    int refCountPage = 0;
    int refPageCount = 0;

    for (auto iter = _mapCache.begin(); iter != _mapCache.end(); iter++) {
      IndexPage* page = iter->second;
      int num = page->GetRefCount();
      if (num > 0) {
        refCountPage++;
        refPageCount += num;
      }
      else if (num < 0) {
        LOG_ERROR << "Errr page ref count. Id=" << page->GetPageId() << "\trefCount=" << num;
      }

      if (!page->Releaseable()) {
        continue;
      }

      if ((int64_t)queue.size() < numDel) {
        queue.push(page);
      }
      else if (page->GetAccessTime() < queue.top()->GetAccessTime()) {
        queue.push(page);
        queue.pop();
      }
    }

    int delCount = 0;
    _rwLock.unlock_shared();
    _rwLock.lock();
    while (queue.size() > 0) {
      IndexPage* page = queue.top();
      queue.pop();

      if (!page->Releaseable()) {
        continue;
      }

      _mapCache.erase(page->HashCode());
      page->GetIndexTree()->DecPages();
      delete page;
      delCount++;
    }
    _rwLock.unlock();

    LOG_DEBUG << "MaxPage=" << _maxCacheSize << "\tUsedPage=" << _mapCache.size()
      << "\tPageDividePool=" << PageDividePool::GetQueuePageSize();
    LOG_DEBUG << "Removed page:" << delCount << "\tWaitWriteQueue:" << StoragePool::GetWaitingWriteTaskCount()
      << "\nRefCountPage=" << refCountPage << "\tRefPageCount=" << refPageCount;
  }
}
