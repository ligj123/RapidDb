#include "CachePool.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Log.h"
#include "../utils/StackTrace.h"
#include <iostream>

namespace storage {
CachePool *CachePool::_gCachePool = []() { return new CachePool; }();
thread_local LocalMap CachePool::_localMap;

#ifdef DEBUG_TEST
vector<unordered_map<uint16_t, vector<Byte *>> *> CachePool::_vctMap;
SpinMutex CachePool::_spinLocal;
unordered_map<Byte *, string> CachePool::_mapApply;
#endif

LocalMap::LocalMap() {
#ifdef DEBUG_TEST
  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  CachePool::_vctMap.push_back(&_map);
#endif
}

LocalMap::~LocalMap() {
  for (auto iter = _map.begin(); iter != _map.end(); iter++) {
    CachePool::BatchRelease(iter->first, iter->second, true);
  }

#ifdef DEBUG_TEST
  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  for (auto iter = CachePool::_vctMap.begin(); iter != CachePool::_vctMap.end();
       iter++) {
    if (*iter == &_map) {
      CachePool::_vctMap.erase(iter);
      return;
    }
  }
#endif

  bStoped = true;
}

void LocalMap::Push(Byte *pBuf, uint16_t eleSize) {
  assert(eleSize % 16 == 0);
  if (bStoped)
    return;

  auto iter = _map.find(eleSize);
  if (iter == _map.end()) {
    _map.insert({eleSize, vector<Byte *>()});
    iter = _map.find(eleSize);
    int num = (eleSize == Configure::GetCachePageSize() ? 32 : 16384 / eleSize);
    if (num < 4)
      num = 4;
    iter->second.reserve(num);
  }

  iter->second.push_back(pBuf);
  if (iter->second.size() >= iter->second.capacity() - 1) {
    CachePool::BatchRelease(iter->first, iter->second, false);
  }
}

Byte *LocalMap::Pop(uint16_t eleSize) {
  assert(eleSize % 8 == 0);

  auto iter = _map.find(eleSize);
  if (iter == _map.end()) {
    _map.insert({eleSize, vector<Byte *>()});
    iter = _map.find(eleSize);
    int num = (eleSize == Configure::GetCachePageSize() ? 32 : 16384 / eleSize);
    if (num < 4)
      num = 4;
    iter->second.reserve(num);
  }

  if (iter->second.size() == 0) {
    CachePool::BatchApply(eleSize, iter->second);
  }

  Byte *buf = iter->second[iter->second.size() - 1];
  iter->second.erase(iter->second.end() - 1);
  return buf;
}

void CachePool::BatchApply(uint32_t bufSize, vector<Byte *> &vct) {
  CachePool *pool = GetInstance();
  auto iter = pool->_mapPool.find(bufSize);
  if (iter == pool->_mapPool.end()) {
    std::unique_lock<SpinMutex> lock(pool->_spinMutex);
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

Buffer *CachePool::AllocateBuffer(uint32_t eleLen) {
  CachePool *pool = GetInstance();
  std::unique_lock<SpinMutex> lock(pool->_spinMutex);

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
  std::unique_lock<SpinMutex> lock(pool->_spinMutex);
  if (pool->_queueFreeBuf.size() > Configure::GetMaxFreeBufferCount()) {
    delete buf;
    pool->_szMemUsed -= Configure::GetCacheBlockSize();
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

#ifdef DEBUG_TEST
size_t CachePool::GetMemoryUsed() {
  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  for (auto mm : _vctMap) {
    for (auto iter = mm->begin(); iter != mm->end(); iter++) {
      BatchRelease(iter->first, iter->second, true);
    }
  }

  auto map = _gCachePool->_mapPool;
  size_t sz = 0;
  for (auto iter = map.begin(); iter != map.end(); iter++) {
    sz += iter->second->UsedMem();
  }
  return sz;
}

Byte *CachePool::ApplyBlock() {
  CachePool *pool = GetInstance();
  unique_lock<SpinMutex> lock(pool->_blockMutex);
  Byte *bys = nullptr;
  if (pool->_queueFreeBlock.size() > 0) {
    bys = pool->_queueFreeBlock.front();
    pool->_queueFreeBlock.pop();
  } else {
    bys = new Byte[Configure::GetResultBlockSize()];
    pool->_totalBlockNum++;
  }

  string str = StackTrace();
  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  _mapApply.emplace(bys, "ApplyBlock\n" + str);
  return bys;
}

void CachePool::ReleaseBlock(Byte *bys) {
  CachePool *pool = GetInstance();
  unique_lock<SpinMutex> lock(pool->_blockMutex);
  if (pool->_queueFreeBlock.size() > Configure::GetMaxNumberFreeResultBlock()) {
    pool->_totalBlockNum--;
    delete bys;
  } else {
    pool->_queueFreeBlock.push(bys);
  }

  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  size_t rt = _mapApply.erase(bys);
  assert(rt == 1);
}
/**Apply a menory block for an index page*/
Byte *CachePool::ApplyPage() {
  CachePool *pool = GetInstance();
  Byte *bys = pool->_localMap.Pop((uint16_t)Configure::GetCachePageSize());

  string str = StackTrace();
  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  _mapApply.emplace(bys, "ApplyPage\n" + str);
  return bys;
}

/**Release a memory block for an index page*/
void CachePool::ReleasePage(Byte *page) {
  CachePool *pool = GetInstance();
  pool->_localMap.Push(page, (uint16_t)Configure::GetCachePageSize());

  unique_lock<SpinMutex> lock(CachePool::_spinLocal);
  size_t rt = _mapApply.erase(page);
  assert(rt == 1);
}

/**Apply a memory block from cache*/
Byte *CachePool::Apply(uint32_t bufSize) {
  assert(bufSize > 0);
  uint32_t sz = CalcBufSize(bufSize);
  if (sz == UINT32_MAX)
    return new Byte[bufSize];
  else {
    CachePool *pool = GetInstance();
    Byte *bys = pool->_localMap.Pop(sz);

    string str = StackTrace();
    unique_lock<SpinMutex> lock(CachePool::_spinLocal);
    _mapApply.emplace(bys, "Apply1 size: " + to_string(bufSize) +
                               "  actual size: " + to_string(sz) + "\n" + str);
    return bys;
  }
}
/**Apply a memory block from cache and set the actual allocated size*/
Byte *CachePool::Apply(uint32_t bufSize, uint32_t &realSize) {
  assert(bufSize > 0);
  realSize = CalcBufSize(bufSize);
  if (realSize == UINT32_MAX) {
    realSize = bufSize;
    return new Byte[realSize];
  } else {
    CachePool *pool = GetInstance();
    Byte *bys = pool->_localMap.Pop((uint16_t)realSize);

    string str = StackTrace();
    unique_lock<SpinMutex> lock(CachePool::_spinLocal);
    _mapApply.emplace(bys, "Apply2:  size: " + to_string(bufSize) +
                               "  actual size: " + to_string(realSize) + "\n" +
                               str);
    return bys;
  }
}
/**Release a memory block with unfixed size*/
void CachePool::Release(Byte *pBuf, uint32_t bufSize) {
  uint32_t sz = CalcBufSize(bufSize);
  if (sz == UINT32_MAX)
    delete[] pBuf;
  else {
    CachePool *pool = GetInstance();
    pool->_localMap.Push(pBuf, (uint16_t)sz);

    unique_lock<SpinMutex> lock(CachePool::_spinLocal);
    size_t rt = _mapApply.erase(pBuf);
    assert(rt == 1);
  }
}
#endif // DEBUG_TEST
} // namespace storage
