#pragma once
#include "../cache/Mallocator.h"
#include "../config/Configure.h"
#include "../core/CachePage.h"
#include "../utils/FastQueue.h"
#include "../utils/Log.h"
#include "../utils/ThreadPool.h"

namespace storage {
using namespace std;

class StoragePool {
public:
  static const uint32_t WRITE_DELAY_MS;
  static const uint64_t MAX_QUEUE_SIZE;

public:
  static void AddTimerTask();
  static void RemoveTimerTask();
  static void WriteCachePage(CachePage *page);
  static size_t GetWaitingPageCount() { return _storagePool->_mapWrite.size(); }
  static void InitPool(ThreadPool *tp) {
    assert(_storagePool == nullptr);
    _storagePool = new StoragePool(tp);
  };
  static void StopPool() {
    delete _storagePool;
    _storagePool = nullptr;
  }

protected:
  static StoragePool *_storagePool;
  static SpinMutex _spinMutex;
  static void PoolManage();

protected:
  StoragePool(ThreadPool *tp)
      : _fastQueue(tp->GetMaxThreads()), _threadPool(tp){};
  MTreeMap<uint64_t, CachePage *>::Type _mapWrite;
  FastQueue<CachePage> _fastQueue;
  ThreadPool *_threadPool;
  friend class StorageTask;
};

class StorageTask : public Task {
  void Run() override {
    unique_lock<SpinMutex> lock(StoragePool::_spinMutex, defer_lock);
    _status = TaskStatus::RUNNING;
    if (lock.try_lock()) {
      StoragePool::PoolManage();
    }
    _status = TaskStatus::INTERVAL;
  }
  bool IsSmallTask() override { return false; }
};
} // namespace storage
