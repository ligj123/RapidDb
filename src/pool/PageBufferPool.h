#pragma once
#include "../core/CachePage.h"
#include "../utils/ConcurrentHashMap.h"
#include "../utils/SpinMutex.h"
#include "../utils/ThreadPool.h"
#include "../utils/TimerThread.h"

namespace storage {
using namespace std;

class PageBufferPool {
public:
  static uint64_t GetMaxCacheSize() { return _maxCacheSize; }
  static void SetMaxCacheSzie(uint64_t size) { _maxCacheSize = (int)size; }

  static void AddPage(CachePage *page);

  static CachePage *GetPage(uint64_t fileId, uint32_t pageId) {
    return GetPage(CachePage::CalcHashCode(fileId, pageId));
  }

  static CachePage *GetPage(uint64_t hashId);
  /**Only used for test to remove results from previous test cases*/
  static void ClearPool();

  static void AddTimerTask();
  static void RemoveTimerTask();

  static uint64_t GetCacheSize() { return _mapCache.Size(); }
  static TaskStatus SetSuspend() { return _taskStatus; }

protected:
  static void PoolManage();

protected:
  static ConcurrentHashMap<uint64_t, CachePage *> _mapCache;
  static SpinMutex _spinMutex;
  // The max cache pages in this pool
  static int64_t _maxCacheSize;
  // To save how many pages have been removed from this pool in previous clean
  // task.
  static int64_t _prevDelNum;
  // Task status
  static TaskStatus _taskStatus;
  friend class PagePoolTask;
};

class PagePoolTask : public Task {
  void Run() override;
  bool IsSmallTask() override { return false; }
};
} // namespace storage
