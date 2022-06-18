#pragma once
#include "../core/IndexPage.h"
#include "../core/IndexTree.h"
#include "../utils/FastQueue.h"
#include "../utils/SpinMutex.h"
#include "../utils/ThreadPool.h"
#include <thread>

namespace storage {
using namespace std;

class PageDividePool {
public:
  static const uint32_t BUFFER_FLUSH_INTEVAL_MS;
  static const uint64_t QUEUE_LIMIT_SIZE;

public:
  static void AddTimerTask();
  static void RemoveTimerTask();
  static void AddCachePage(IndexPage *page);
  static void PoolManage();

  static size_t GetTaskCount() { return _divPool->_mapPage.size(); }
  static void InitPool(ThreadPool *tp) {
    assert(_divPool == nullptr);
    _divPool = new PageDividePool(tp);
  }
  static void StopPool();

protected:
  static SpinMutex _spinMutex;
  static PageDividePool *_divPool;

protected:
  PageDividePool(ThreadPool *tp)
      : _fastQueue(tp->GetMaxThreads()), _threadPool(tp) {}
  MTreeMap<uint64_t, IndexPage *>::Type _mapPage;
  FastQueue<IndexPage> _fastQueue;
  ThreadPool *_threadPool;
  friend class PageDivideTask;
};

class PageDivideTask : public Task {
  void Run() override {
    _status = TaskStatus::RUNNING;
    unique_lock<SpinMutex> lock(PageDividePool::_spinMutex, defer_lock);
    if (lock.try_lock()) {
      PageDividePool::PoolManage();
    }
    _status = TaskStatus::INTERVAL;
  }

  bool IsSmallTask() override { return false; }
};
} // namespace storage
