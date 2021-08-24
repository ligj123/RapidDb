#include "CachePool.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include <string>

namespace storage {
CachePool *CachePool::_gCachePool = []() { return new CachePool; }();
thread_local LocalMap CachePool::_localMap;

LocalMap::~LocalMap() {
  for (auto iter = _map.begin(); iter != _map.end(); iter++) {
    CachePool::BatchRelease(iter->first, iter->second, true);
  }
}

void LocalMap::Push(Byte *pBuf, uint32_t eleSize) {
  assert(eleSize % 8 == 0);

  auto iter = _map.find(eleSize);
  if (iter == _map.end()) {
    _map.insert({eleSize, vector<Byte *>()});
    iter = _map.find(eleSize);
    iter->second.reserve(eleSize <= 512 ? 1024 : 20);
  }

  iter->second.push_back(pBuf);
  if (iter->second.size() >= iter->second.capacity() - 1) {
    CachePool::BatchRelease(iter->first, iter->second, false);
  }
}

Byte *LocalMap::Pop(uint32_t eleSize) {
  assert(eleSize % 8 == 0);
  auto iter = _map.find(eleSize);
  if (iter == _map.end()) {
    _map.insert({eleSize, vector<Byte *>()});
    iter = _map.find(eleSize);
    iter->second.reserve(eleSize <= 512 ? 400 : 20);
  }

  if (iter->second.size() == 0) {
    CachePool::BatchApply(eleSize, iter->second);
  }

  Byte *buf = iter->second[iter->second.size() - 1];
  iter->second.erase(iter->second.end() - 1);
  return buf;
}

Byte *CachePool::ApplyPage() {
  CachePool *pool = GetInstance();
  return pool->_localMap.Pop((uint32_t)Configure::GetCachePageSize());
}

void CachePool::ReleasePage(Byte *page) {
  CachePool *pool = GetInstance();
  pool->_localMap.Push(page, (uint32_t)Configure::GetCachePageSize());
}

Byte *CachePool::Apply(uint32_t eleSize) {
  CachePool *pool = GetInstance();
  return pool->_localMap.Pop(eleSize);
}

void CachePool::BatchApply(uint32_t bufSize, vector<Byte *> &vct) {
  CachePool *pool = GetInstance();
  auto iter = pool->_mapPool.find(bufSize);
  if (iter == pool->_mapPool.end()) {
    std::unique_lock<utils::SpinMutex> lock(pool->_spinMutex);
    iter = pool->_mapPool.find(bufSize);
    if (iter == pool->_mapPool.end()) {
      pool->_mapPool.insert(
          pair<uint32_t, BufferPool *>(bufSize, new BufferPool(bufSize)));
      iter = pool->_mapPool.find(bufSize);
    }
  }

  iter->second->Apply(vct);
}

void CachePool::BatchRelease(uint32_t bufSize, vector<Byte *> &vct, bool bAll) {
  CachePool *pool = GetInstance();
  auto iter = pool->_mapPool.find(bufSize);
  assert(iter != pool->_mapPool.end());

  iter->second->Release(vct, bAll);
}

void CachePool::Release(Byte *pBuf, uint32_t eleSize) {
  CachePool *pool = GetInstance();
  pool->_localMap.Push(pBuf, eleSize);
}

Buffer *CachePool::AllocateBuffer(uint32_t eleLen) {
  CachePool *pool = GetInstance();
  std::unique_lock<utils::SpinMutex> lock(pool->_spinMutex);

  if (pool->_queueFreeBuf.size() > 0) {
    Buffer *buf = GetInstance()->_queueFreeBuf.front();
    buf->Init(eleLen);
    GetInstance()->_queueFreeBuf.pop();
    return buf;
  }

  pool->_szMemUsed += Configure::GetCacheBlockSize();
  return new Buffer(eleLen);
}

void CachePool::RecycleBuffer(Buffer *buf) {
  CachePool *pool = GetInstance();
  std::unique_lock<utils::SpinMutex> lock(pool->_spinMutex);
  if (pool->_queueFreeBuf.size() > Configure::GetMaxFreeBufferCount()) {
    delete buf;
  } else {
    pool->_queueFreeBuf.push(buf);
  }
}

CachePool::CachePool() : _szMemUsed(0), _mapPool(500) {}

CachePool::~CachePool() {
  while (_queueFreeBuf.size() > 0) {
    free(_queueFreeBuf.front());
    _queueFreeBuf.pop();
  }
}
} // namespace storage
