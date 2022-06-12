#include "StoragePool.h"
#include "../core/CachePage.h"
#include "../core/IndexTree.h"

namespace storage {
const uint32_t StoragePool::WRITE_DELAY_MS = 1 * 1000;
const uint64_t StoragePool::MAX_QUEUE_SIZE =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
StoragePool *StoragePool::_storagePool = nullptr;
SpinMutex StoragePool::_spinMutex;

void StoragePool::AddTimerTask() {
  TimerThread::AddCircleTask("StoragePool", 1000000, []() {
    StorageTask *task = new StorageTask();
    _storagePool->_threadPool->AddTask(task);
  });
}
void StoragePool::RemoveTimerTask() { TimerThread::RemoveTask("StoragePool"); }

void StoragePool::WriteCachePage(CachePage *page) {
  _storagePool->_fastQueue.Push(page);
}

void StoragePool::PoolManage() {
  queue<CachePage *> q;
  _storagePool->_fastQueue.swap(q);
  while (q.size() > 0) {
    CachePage *page = q.front();
    q.pop();

    if (_storagePool->_mapWrite.find(page->HashCode()) !=
        _storagePool->_mapWrite.end()) {
      page->DecRef();
    }

    page->UpdateWriteTime();
    _storagePool->_mapWrite.insert({page->HashCode(), page});
  }

  bool bmax = (_storagePool->_mapWrite.size() < MAX_QUEUE_SIZE / 2);
  for (auto iter = _storagePool->_mapWrite.begin();
       iter != _storagePool->_mapWrite.end(); iter++) {
    CachePage *page = iter->second;
    if (!page->GetIndexTree()->IsClosed() && bmax &&
        (MicroSecTime() - page->GetWriteTime() < WRITE_DELAY_MS)) {
      continue;
    }

    if (page->IsDirty()) {
      page->WritePage();
    }

    page->DecRef();
    _storagePool->_mapWrite.erase(iter);
  }
}
} // namespace storage
