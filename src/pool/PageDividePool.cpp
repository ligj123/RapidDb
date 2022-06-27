﻿#include "PageDividePool.h"
#include "../config/Configure.h"

namespace storage {
const uint32_t PageDividePool::BUFFER_FLUSH_INTEVAL_MS = 10 * 1000;
const uint64_t PageDividePool::QUEUE_LIMIT_SIZE = 1000;
SpinMutex PageDividePool::_spinMutex;
PageDividePool *PageDividePool::_divPool = nullptr;

void PageDividePool::AddTimerTask() {
  TimerThread::AddCircleTask("PageDividePool", 1000000, []() {
    PageDivideTask *task = new PageDivideTask();
    _divPool->_threadPool->AddTask(task);
  });
}
void PageDividePool::RemoveTimerTask() {
  TimerThread::RemoveTask("PageDividePool");
}

void PageDividePool::AddCachePage(IndexPage *page) {
  _divPool->_fastQueue.Push(page);
}

void PageDividePool::StopPool() {
  while (!_divPool->_fastQueue.Empty()) {
    PoolManage();
  }
  delete _divPool;
  _divPool = nullptr;
}

void PageDividePool::PushTask() {
  PageDivideTask *task = new PageDivideTask();
  _divPool->_threadPool->AddTask(task);
}

void PageDividePool::PoolManage() {
  queue<IndexPage *> q;
  _divPool->_fastQueue.Swap(q);
  while (q.size() > 0) {
    IndexPage *page = q.front();
    q.pop();

    if (_divPool->_mapPage.find(page->HashCode()) != _divPool->_mapPage.end()) {
      page->DecRef();
    }

    page->UpdateWriteTime();
    _divPool->_mapPage.insert({page->HashCode(), page});
  }

  bool unfull = (_divPool->_mapPage.size() < QUEUE_LIMIT_SIZE);
  for (auto iter = _divPool->_mapPage.begin();
       iter != _divPool->_mapPage.end();) {
    auto page = iter->second;
    if (page->GetTranCount() > 0UL ||
        (unfull && !page->IsOverTime(BUFFER_FLUSH_INTEVAL_MS) &&
         !page->IsOverlength() && !page->GetIndexTree()->IsClosed())) {
      continue;
    }

    bool b = page->WriteTryLock();
    if (!b || page->GetTranCount() > 0UL) {
      if (b)
        page->WriteUnlock();
      continue;
    }

    bool bPassed = false;
    if (page->GetTotalDataLength() > page->GetMaxDataLength()) {
      bPassed = page->PageDivide();
    } else {
      bPassed = page->SaveRecords();
    }

    page->WriteUnlock();
    if (bPassed) {
      page->DecRef();
      iter = _divPool->_mapPage.erase(iter);
    } else {
      iter++;
    }
  }
}
} // namespace storage
