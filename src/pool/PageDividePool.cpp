#include "PageDividePool.h"
#include "../config/Configure.h"
#include "StoragePool.h"

namespace storage {
const uint32_t PageDividePool::BUFFER_FLUSH_INTEVAL_MS = 10 * 1000;
const uint64_t PageDividePool::QUEUE_LIMIT_SIZE = 1000;
SpinMutex PageDividePool::_spinMutex;
PageDividePool *PageDividePool::_divPool = nullptr;

void PageDividePool::AddTimerTask() {
  TimerThread::AddCircleTask("PageDividePool", 1000000, []() {
    assert(_divPool != nullptr);
    bool b = _divPool->_bInThreadPool.exchange(true, memory_order_relaxed);
    if (b)
      return;

    PageDivideTask *task = new PageDivideTask();
    _divPool->_threadPool->AddTask(task);
  });
}
void PageDividePool::RemoveTimerTask() {
  TimerThread::RemoveTask("PageDividePool");
}

void PageDividePool::StopPool() {
  while (!_divPool->_fastQueue.RoughEmpty()) {
    PoolManage();
  }
  delete _divPool;
  _divPool = nullptr;
}

void PageDividePool::PushTask() {
  assert(_divPool != nullptr);
  bool b = _divPool->_bInThreadPool.exchange(true, memory_order_relaxed);
  if (b)
    return;
  PageDivideTask *task = new PageDivideTask();
  _divPool->_threadPool->AddTask(task);
}

void PageDividePool::PoolManage() {
  queue<IndexPage *> q;
  _divPool->_fastQueue.Pop(q);
  while (q.size() > 0) {
    IndexPage *page = q.front();
    q.pop();

    if (_divPool->_mapPage.find(page->HashCode()) != _divPool->_mapPage.end()) {
      page->DecRef();
    } else {
      page->UpdateDividTime();
      _divPool->_mapPage.insert({page->HashCode(), page});
    }
  }

  bool unfull = (_divPool->_mapPage.size() < QUEUE_LIMIT_SIZE);
  for (auto iter = _divPool->_mapPage.begin();
       iter != _divPool->_mapPage.end();) {
    auto page = iter->second;
    if (page->GetTranCount() > 0UL || page->GetWaitTasks().size() > 0 ||
        (unfull && !page->IsDividOverTime(BUFFER_FLUSH_INTEVAL_MS) &&
         !page->IsOverlength() && !page->GetIndexTree()->IsClosed())) {
      iter++;
      continue;
    }

    bool b = page->WriteTryLock();
    if (!b || page->GetTranCount() > 0UL) {
      if (b)
        page->WriteUnlock();
      iter++;
      continue;
    }

    bool bPassed = false;
    if (page->GetTotalDataLength() > page->GetMaxDataLength()) {
      page->PageDivide();
      iter++;
    } else {
      page->SetInDivid(false);
      bPassed = page->SaveRecords();
      if (bPassed) {
        StoragePool::AddPage(page, false);
        iter = _divPool->_mapPage.erase(iter);
      } else {
        page->SetInDivid(true);
        iter++;
      }
    }

    page->WriteUnlock();
  }

  _divPool->_bInThreadPool.store(false);
}
} // namespace storage
