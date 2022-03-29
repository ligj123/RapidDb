#pragma once
#include "../core/IndexPage.h"
#include "../utils/SpinMutex.h"

namespace storage {
using namespace std;
class PageBufferPool {

public:
  static uint64_t GetMaxCacheSize() { return _maxCacheSize; }
  static void SetMaxCacheSzie(uint64_t size) { _maxCacheSize = (int)size; }

  static void AddPage(IndexPage *page);

  static IndexPage *GetPage(uint64_t fileId, uint32_t pageId) {
    return GetPage(CachePage::CalcHashCode(fileId, pageId));
  }

  static IndexPage *GetPage(uint64_t hashId);
  /**Only used for test to remove results from previous test cases*/
  static void ClearPool();

  static uint64_t GetCacheSize() { return _mapCache.size(); }
  static void SetSuspend(bool b) { _bSuspend = true; }

protected:
  static void PoolManage();
  static thread *CreateThread();

protected:
  static MHashMap<uint64_t, IndexPage *>::Type _mapCache;
  static SharedSpinMutex _rwLock;
  static int64_t _maxCacheSize;
  static thread _tIndexPageManager;
  static int64_t _prevDelNum;
  static thread *_pageBufferThread;
  static bool _bSuspend;
  static bool _bStop;
};
} // namespace storage
