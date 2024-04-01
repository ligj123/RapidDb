#pragma once
#include "../core/CachePage.h"
#include "../utils/SpinMutex.h"
#include "../utils/TimerThread.h"

#include <thread>
#include <unordered_map>

namespace storage {
using namespace std;
class PagePoolTask;

class CachePagePool {
public:
  static uint64_t GetMaxCacheSize() { return _maxCacheSize; }
  static void SetMaxCacheSize(uint64_t sz) { _maxCacheSize = sz; }

  static void AddPage(CachePage *page);

  static CachePage *GetPage(uint64_t fileId, uint32_t pageId) {
    return GetPage(CachePage::CalcHashCode(fileId, pageId));
  }

  static CachePage *GetPage(uint64_t hashId);

  static uint64_t GetCacheSize() { return _mapCache.size(); }
  static void InitPool();
  static void StopPool();
  static void SetUrgentTask() { _urgentTask.store(true, memory_order_relaxed); }
  static void ClearPool();

protected:
  static void PoolManage();

protected:
  // The max cache pages in this pool
  static uint64_t _maxCacheSize;
  static unordered_map<uint64_t, CachePage *> _mapCache;
  static SpinMutex _spinMutex;
  // If here has urgent task that need to clear all pages in a index tree.
  static atomic_bool _urgentTask;
  static thread *_thread;
  // This thread has stoped or not.
  static bool _bStoped;
  // To record the cycle times in thread.
  static uint64_t _countPool;
};
} // namespace storage
