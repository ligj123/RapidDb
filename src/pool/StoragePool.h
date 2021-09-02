#pragma once
#include "../cache/Mallocator.h"
#include "../config/Configure.h"
#include "../core/CachePage.h"
#include "../utils/Log.h"
#include "../utils/ThreadPool.h"
#include <queue>

namespace storage {
using namespace std;
class PageReadTask : public Task {
public:
  PageReadTask(CachePage *page) : _page(page) {}
  void Run() override {
    _page->ReadPage();
    _promise.set_value(0);
  }
  bool IsSmallTask() override { return false; }

protected:
  CachePage *_page;
};

class StoragePool {
public:
  static void WriteCachePage(CachePage *page);
  static future<int> ReadCachePage(CachePage *page);
  static void FlushWriteCachePage() { _bWriteFlush = true; }
  static void SetWriteSuspend(bool b) { _bWriteSuspend = b; }
  static uint32_t GetWaitingWriteTaskCount() {
    return (uint32_t)_mapWrite.size();
  }
  static void SetStop() { _bStop = true; }

protected:
  static thread *CreateWriteThread();

protected:
  static const uint32_t WRITE_DELAY_MS;
  static const uint64_t MAX_QUEUE_SIZE;
  static ThreadPool _threadReadPool;

  static MHashMap<uint64_t, CachePage *>::Type _mapTmp;
  static MTreeMap<uint64_t, CachePage *>::Type _mapWrite;
  static thread *_threadWrite;
  static bool _bWriteFlush;
  static bool _bReadFirst;
  static SpinMutex _spinMutex;
  static bool _bWriteSuspend;
  static bool _bStop;
};
} // namespace storage
