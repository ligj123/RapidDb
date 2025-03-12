#pragma once
#include "../core/IndexPage.h"
#include "../core/IndexTree.h"
#include "../utils/SpinMutex.h"
#include "../utils/TimerThread.h"

#include <thread>
#include <unordered_map>

namespace storage {
using namespace std;

class CachePagePool {
public:
  static uint64_t GetMaxCacheSize() { return _maxCacheSize; }
  static void SetMaxCacheSize(uint64_t sz) { _maxCacheSize = sz; }

  static void AddPage(IndexPage *page);
  static void AddPages(MVector<IndexPage *> &vctPage);

  static IndexPage *GetPage(IndexTree *idxTree, PageID pageId, PageType type);
  static MVector<IndexPage *> GetPages(IndexTree *idxTree, PageType type,
                                       MVector<PageID> &vctPageId);

  static uint64_t GetCacheSize() { return _mapCache.size(); }

  /**
   *@brief Clear all page in _mapCache, only used for test aim.
   */
  static void ClearPool();

protected:
  static void PoolManage(MemoryStatus status);

protected:
  // The max cache pages in this pool
  static uint64_t _maxCacheSize;
  static unordered_map<uint64_t, IndexPage *> _mapCache;
  static SpinMutex _spinMutex;
  static uint64_t _midPage;

  friend class CachePagePoolTask;
};

class CachePagePoolTask : public ThreadTask {
public:
  CachePagePoolTask() : ThreadTask(nullptr) { _taskName = "CachePagePoolTask"; }
  static void SetStop() { _instance->_bStoped = true; }
  static CachePagePoolTask *GetInstance() {
    assert(_instance != nullptr);
    return _instance;
  }
  static void Init(ThreadPool *tpool);

public:
  TaskStatus Run() override;

protected:
  static CachePagePoolTask *_instance;
  // This thread has stoped or not.
  bool _bStoped;
  // The last datetime to handle CachePagePool.
  DT_MicroSec _dtLastVisit{0};
};
} // namespace storage
