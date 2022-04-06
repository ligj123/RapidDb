#include "StoragePool.h"
#include "../core/CachePage.h"
#include "../core/IndexTree.h"

namespace storage {
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

          if (page->DecRefCount() == 1) {
            delete page;
          }
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
  PageReadTask *task = new PageReadTask(page);
  _threadReadPool.AddTask(task);

  return task->GetFuture();
}
} // namespace storage
