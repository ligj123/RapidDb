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
    assert(_storagePool != nullptr);
    bool b = _storagePool->_bInThreadPool.exchange(true, memory_order_relaxed);
    if (b)
      return;

    StorageTask *task = new StorageTask();
    _storagePool->_threadPool->AddTask(task);
  });
}
void StoragePool::RemoveTimerTask() { TimerThread::RemoveTask("StoragePool"); }

void StoragePool::PushTask() {
  assert(_storagePool != nullptr);
  bool b = _storagePool->_bInThreadPool.exchange(true, memory_order_relaxed);
  if (b)
    return;

  StorageTask *task = new StorageTask();
  _storagePool->_threadPool->AddTask(task);
}

void StoragePool::StopPool() {
  if (!_storagePool->_fastQueue.Empty()) {
    PoolManage();
  }

  delete _storagePool;
  _storagePool = nullptr;
}

void StoragePool::PoolManage() {
  queue<CachePage *> q;
  _storagePool->_fastQueue.Swap(q);
  while (q.size() > 0) {
    CachePage *page = q.front();
    q.pop();

    if (_storagePool->_mapWrite.find(page->HashCode()) !=
        _storagePool->_mapWrite.end()) {
      page->DecRef();
    } else {
      page->UpdateWriteTime();
      _storagePool->_mapWrite.insert({page->HashCode(), page});
    }
  }

  bool bmax = (_storagePool->_mapWrite.size() > MAX_QUEUE_SIZE / 2);
  IndexTree *tree = nullptr;
  PageFile *pf = nullptr;

  for (auto iter = _storagePool->_mapWrite.begin();
       iter != _storagePool->_mapWrite.end();) {
    CachePage *page = iter->second;
    if (!page->GetIndexTree()->IsClosed() && bmax &&
        !page->IsWriteOverTime(WRITE_DELAY_MS)) {
      iter++;
      continue;
    }

    page->SetInStorage(false);
    if (page->IsDirty()) {
      if (tree == nullptr || tree != page->GetIndexTree()) {
        if (pf != nullptr) {
          tree->ReleasePageFile(pf);
        }

        tree = page->GetIndexTree();
        pf = tree->ApplyPageFile();
      }
      page->WritePage(pf);
    }

    page->DecRef();
    iter = _storagePool->_mapWrite.erase(iter);
  }

  if (tree != nullptr) {
    tree->ReleasePageFile(pf);
  }

  _storagePool->_bInThreadPool.store(false, memory_order_relaxed);
}
} // namespace storage
