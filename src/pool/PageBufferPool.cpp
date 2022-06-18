#include "PageBufferPool.h"
#include "../core/IndexTree.h"
#include "../utils/Log.h"
#include "PageDividePool.h"
#include "StoragePool.h"
#include <forward_list>
#include <shared_mutex>

namespace storage {
SpinMutex PageBufferPool::_spinMutex;
int64_t PageBufferPool::_maxCacheSize =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
int64_t PageBufferPool::_prevDelNum = 100;
TaskStatus PageBufferPool::_taskStatus = TaskStatus::UNINIT;
ConcurrentHashMap<uint64_t, CachePage *>
    PageBufferPool::_mapCache(100, PageBufferPool::_maxCacheSize);

void PageBufferPool::AddPage(CachePage *page) {
  _mapCache.Insert(page->HashCode(), page);
}

CachePage *PageBufferPool::GetPage(uint64_t hashId) {
  CachePage *page = nullptr;
  _mapCache.Find(hashId, page);
  return page;
}

void PageBufferPool::ClearPool() {
  for (int i = 0; i < _mapCache.GetGroupCount(); i++) {
    _mapCache.Clear(i);
  }
}

void PageBufferPool::PoolManage() {
  int64_t numDel = _mapCache.Size() - _maxCacheSize * 4 / 5;
  if (numDel <= 0) {
    LOG_INFO << "MaxPage=" << _maxCacheSize
             << "\tUsedPage=" << _mapCache.Size();
    //  << "\tPageDividePool=" << PageDividePool::GetWaitingPageSize();
    // LOG_INFO << "\tWaitWriteQueue:" <<
    // StoragePool::GetWaitingWriteTaskCount();
    return;
  }

  if (numDel > _prevDelNum * 2) {
    numDel = _prevDelNum * 2;
    if (numDel > 100000)
      numDel = 100000;
  } else if (numDel < _prevDelNum / 2) {
    numDel = _prevDelNum / 2;
    if (numDel < 1000)
      numDel = 1000;
  }

  _prevDelNum = numDel;
  int64_t grDel = numDel / _mapCache.GetGroupCount();
  int delCount = 0;

  static auto cmp = [](CachePage *left, CachePage *right) {
    return left->GetAccessTime() < right->GetAccessTime();
  };

  for (int i = 0; i < _mapCache.GetGroupCount(); i++) {
    priority_queue<CachePage *, MDeque<CachePage *>::Type, decltype(cmp)> queue(
        cmp);
    forward_list<CachePage *> flist;

    for (auto iter = _mapCache.Begin(i); iter != _mapCache.End(i); iter++) {
      CachePage *page = iter->second;
      if (page->GetIndexTree()->IsClosed()) {
        flist.push_front(page);
        continue;
      }

      if (!page->Releaseable()) {
        continue;
      }

      if ((int64_t)queue.size() < grDel) {
        queue.push(page);
      } else if (page->GetAccessTime() < queue.top()->GetAccessTime()) {
        queue.push(page);
        queue.pop();
      }
    }

    _mapCache.Lock(i);

    for (CachePage *page : flist) {
      _mapCache.Erase(i, page->HashCode());
      page->DecRef();
    }

    while (queue.size() > 0) {
      CachePage *page = queue.top();
      queue.pop();

      if (!page->Releaseable()) {
        continue;
      }

      _mapCache.Erase(i, page->HashCode());
      page->DecRef();
      delCount++;
    }
    _mapCache.Unlock(i);
  }

  LOG_INFO << "MaxPage=" << _maxCacheSize << "\tUsedPage=" << _mapCache.Size()
           << "Removed page:" << delCount;

  // LOG_INFO << "\tPageDividePool=" << PageDividePool::GetWaitingPageSize()
  //          << "\tWaitWriteQueue:" << StoragePool::GetWaitingWriteTaskCount()
  //          << "\tRefCountPage=" << refCountPage
  //          << "\tRefPageCount=" << refPageCount;
}

void PageBufferPool::AddTimerTask() {
  TimerThread::AddCircleTask("PageBufferPool", 5000000, []() {
    PagePoolTask *task = new PagePoolTask();
    ThreadPool::InstMain().AddTask(task);
  });
}

void PageBufferPool::RemoveTimerTask() {
  TimerThread::RemoveTask("PageBufferPool");
}

void PagePoolTask::Run() {
  unique_lock<SpinMutex> lock(PageBufferPool::_spinMutex, defer_lock);
  PageBufferPool::_taskStatus = TaskStatus::RUNNING;
  _status = TaskStatus::RUNNING;

  if (lock.try_lock()) {
    PageBufferPool::PoolManage();
  }

  PageBufferPool::_taskStatus = TaskStatus::INTERVAL;
  _status = TaskStatus::INTERVAL;
}
} // namespace storage
