#include "StoragePool.h"
#include "../core/CachePage.h"
#include "../core/IndexTree.h"

namespace storage {
const uint32_t StoragePool::WRITE_DELAY_MS = 1 * 1000;
const uint64_t StoragePool::MAX_QUEUE_SIZE = Configure::GetTotalCacheSize() / Configure::GetCachePageSize();

utils::ThreadPool StoragePool::_threadReadPool("StoragePool", (uint32_t)MAX_QUEUE_SIZE, 1);

unordered_map<uint64_t, CachePage*> StoragePool::_mapTmp1;
unordered_map<uint64_t, CachePage*> StoragePool::_mapTmp2;
map<uint64_t, CachePage*> StoragePool::_mapWrite;
thread* StoragePool::_threadWrite = StoragePool::CreateWriteThread();
bool StoragePool::_bWriteFlush = false;
bool StoragePool::_bReadFirst = true;
utils::SpinMutex StoragePool::_spinMutex;
bool StoragePool::_bWriteSuspend = false;

thread* StoragePool::CreateWriteThread() {
  thread* t = new thread([]() {
    utils::ThreadPool::_threadName = "WriteThread";
    while (true) {
      try {
        {
          unique_lock<utils::SpinMutex> lock(_spinMutex);
          _mapTmp2.swap(_mapTmp1);
        }

        for (auto iter = _mapTmp2.begin(); iter != _mapTmp2.end(); iter++) {
          if (_mapWrite.find(iter->first) != _mapWrite.end()) {
            continue;
          }

          iter->second->IncRefCount();
          iter->second->UpdateWriteTime();
          _mapWrite.insert({ iter->first, iter->second });
        }

        _mapTmp2.clear();

        if ((_bReadFirst && _threadReadPool.GetTaskCount() > 0) ||
          _mapWrite.size() == 0 || _bWriteSuspend) {
          if (_mapWrite.size() == 0) _bWriteFlush = false;

          this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }

        if (!_bWriteFlush) {
          if (_mapWrite.size() < MAX_QUEUE_SIZE / 4) {
            this_thread::sleep_for(std::chrono::milliseconds(1));
          }
        }

        for (auto iter = _mapWrite.begin(); iter != _mapWrite.end();) {
          auto iter2 = iter;
          CachePage* page = iter->second;
          if (!_bWriteFlush && (_mapWrite.size() < MAX_QUEUE_SIZE / 2) && !page->IsFileClosed()
            && (CachePage::GetMsFromEpoch() - page->GetWriteTime() < WRITE_DELAY_MS)) {
            continue;
          }

          if (page->IsDirty()) {
            page->WritePage();
          }

          page->DecRefCount();
        }
      } catch (...)
      {
        LOG_ERROR << "unknown exception!";
      }
    }
    });
  return t;
}

void StoragePool::WriteCachePage(CachePage* page) {
  {
    lock_guard< utils::SpinMutex> lock(_spinMutex);
    if (_mapTmp1.find(page->HashCode()) != _mapTmp1.end()) {
      return;
    }

    _mapTmp1.insert({ page->HashCode(), page });
  }

  while (_mapWrite.size() > MAX_QUEUE_SIZE * 2) {
    this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

future<int> StoragePool::ReadCachePage(CachePage* page) {
  future<int> fut = _threadReadPool.AddTask([page]() {
    page->ReadPage();
    return 1;
    });

  return fut;
}

}
