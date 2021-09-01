#include "StoragePool.h"
#include "../core/CachePage.h"
#include "../core/IndexTree.h"

namespace storage {
const uint32_t StoragePool::WRITE_DELAY_MS = 1 * 1000;
const uint64_t StoragePool::MAX_QUEUE_SIZE =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();

ThreadPool StoragePool::_threadReadPool("StoragePool", (uint32_t)MAX_QUEUE_SIZE,
                                        1);

MHashMap<uint64_t, CachePage *>::Type StoragePool::_mapTmp;
MTreeMap<uint64_t, CachePage *>::Type StoragePool::_mapWrite;
thread *StoragePool::_threadWrite = StoragePool::CreateWriteThread();
bool StoragePool::_bWriteFlush = false;
bool StoragePool::_bReadFirst = true;
SpinMutex StoragePool::_spinMutex;
bool StoragePool::_bWriteSuspend = false;
bool StoragePool::_bStop = false;

thread *StoragePool::CreateWriteThread() {
  thread *t = new thread([]() {
    ThreadPool::_threadName = "WriteThread";
    while (true) {
      try {
        MHashMap<uint64_t, CachePage *>::Type mapTmp2;
        if (_mapTmp.size() > 0) {
          unique_lock<SpinMutex> lock(_spinMutex);
          mapTmp2.swap(_mapTmp);
        }

        for (auto iter = mapTmp2.begin(); iter != mapTmp2.end(); iter++) {
          if (_mapWrite.find(iter->first) != _mapWrite.end()) {
            iter->second->DecRefCount();
            continue;
          }

          _mapWrite.insert({iter->first, iter->second});
        }

        if ((_bReadFirst && _threadReadPool.GetTaskCount() > 0) ||
            _mapWrite.size() == 0 || _bWriteSuspend) {
          if (_mapWrite.size() == 0) {
            if (_bStop)
              break;
            _bWriteFlush = false;
          }

          this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }

        if (!_bWriteFlush) {
          if (_mapWrite.size() < MAX_QUEUE_SIZE / 4) {
            this_thread::sleep_for(std::chrono::milliseconds(1));
          }
        }

        for (auto iter = _mapWrite.begin(); iter != _mapWrite.end();) {
          auto iter2 = iter++;
          CachePage *page = iter2->second;
          if (!_bWriteFlush && (_mapWrite.size() < MAX_QUEUE_SIZE / 2) &&
              !page->IsFileClosed() &&
              (CachePage::GetMsFromEpoch() - page->GetWriteTime() <
               WRITE_DELAY_MS)) {
            continue;
          }

          if (page->IsDirty()) {
            page->WritePage();
          }

          page->DecRefCount();
          _mapWrite.erase(iter2);
        }
      } catch (...) {
        LOG_ERROR << "unknown exception!";
      }
    }
  });
  return t;
}

void StoragePool::WriteCachePage(CachePage *page) {
  {
    lock_guard<SpinMutex> lock(_spinMutex);
    if (_mapTmp.find(page->HashCode()) != _mapTmp.end()) {
      return;
    }

    page->IncRefCount();
    page->UpdateWriteTime();
    _mapTmp.insert({page->HashCode(), page});
  }

  while (_mapWrite.size() > MAX_QUEUE_SIZE * 2) {
    this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

future<int> StoragePool::ReadCachePage(CachePage *page) {
  future<int> fut = _threadReadPool.AddTask([page]() {
    page->ReadPage();
    return 1;
  });

  return fut;
}

} // namespace storage
