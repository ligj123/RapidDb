#include "cache/CachePool.h"
#include "core/IndexTree.h"
#include "pool/PageBufferPool.h"
#include "pool/PageDividePool.h"
#include "pool/StoragePool.h"

namespace storage {
// class CachePool
CachePool *CachePool::_gCachePool = []() {
  cout << "_gCachePool" << endl;
  return new CachePool;
}();
thread_local LocalMap CachePool::_localMap;

// class IndexTree
MHashSet<uint16_t>::Type IndexTree::_setFiledId;

// class PageBufferPool
MHashMap<uint64_t, IndexPage *>::Type PageBufferPool::_mapCache;
SharedSpinMutex PageBufferPool::_rwLock;
int64_t PageBufferPool::_maxCacheSize =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
thread PageBufferPool::_tIndexPageManager;
int64_t PageBufferPool::_prevDelNum = 100;
thread *PageBufferPool::_pageBufferThread = PageBufferPool::CreateThread();
bool PageBufferPool::_bSuspend = false;
bool PageBufferPool::_bStop = false;

// class PageDividePool
const uint64_t PageDividePool::MAX_QUEUE_SIZE =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
const uint32_t PageDividePool::BUFFER_FLUSH_INTEVAL_MS = 1 * 1000;
const int PageDividePool::SLEEP_INTEVAL_MS = 100;

MTreeMap<uint64_t, IndexPage *>::Type PageDividePool::_mapPage;
MHashMap<uint64_t, IndexPage *>::Type PageDividePool::_mapTmp;
thread *PageDividePool::_treeDivideThread = PageDividePool::CreateThread();
bool PageDividePool::_bSuspend = false;
SpinMutex PageDividePool::_spinMutex;
bool PageDividePool::_bStop = false;

// class StoragePool
const uint32_t StoragePool::WRITE_DELAY_MS = 1 * 1000;
const uint64_t StoragePool::MAX_QUEUE_SIZE =
    Configure::GetTotalCacheSize() / Configure::GetCachePageSize();
ThreadPool StoragePool::_threadReadPool("StoragePool", (uint32_t)MAX_QUEUE_SIZE,
                                        1, 1);
MHashMap<uint64_t, CachePage *>::Type StoragePool::_mapTmp;
MTreeMap<uint64_t, CachePage *>::Type StoragePool::_mapWrite;
thread *StoragePool::_threadWrite = StoragePool::CreateWriteThread();
bool StoragePool::_bWriteFlush = false;
bool StoragePool::_bReadFirst = true;
SpinMutex StoragePool::_spinMutex;
bool StoragePool::_bWriteSuspend = false;
bool StoragePool::_bStop = false;
} // namespace storage
