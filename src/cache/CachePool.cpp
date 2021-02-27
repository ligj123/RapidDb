#include "CachePool.h"
#include "../utils/ErrorMsg.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include <string>

namespace storage {
  CachePool* CachePool::_gCachePool = []() {
    return new CachePool;
  }();

  set<uint32_t> CachePool::_gSetBufSize = { 30, 100, 300, 1000, 2000, 4000, 8000, 16000 };

  Byte* CachePool::ApplyPage() {
    CachePool* pool = GetInstance();
    std::unique_lock<utils::SpinMutex> lock(pool->_spinMutexPage);
    if (pool->_queueFreePage.size() > 0) {
      Byte* bys = pool->_queueFreePage.front();
      pool->_queueFreePage.pop();
      return bys;
    } else {
      lock.unlock();
      return new Byte[Configure::GetCachePageSize()];
    }
  }

  void CachePool::ReleasePage(Byte* page) {
    CachePool* pool = GetInstance();
    std::unique_lock< utils::SpinMutex> lock(pool->_spinMutexPage);
    if (pool->_queueFreePage.size() > Configure::GetMaxFreeBufferCount()) {
      lock.unlock();
      delete[] page;
    } else {
      pool->_queueFreePage.push(page);
    }
  }

  Byte* CachePool::ApplyOverflowCache() {
    CachePool* pool = GetInstance();
    std::unique_lock< utils::SpinMutex> lock(pool->_spinMutexOvf);
    if (pool->_queueFreeOvf.size() > 0) {
      Byte* bys = pool->_queueFreeOvf.front();
      pool->_queueFreeOvf.pop();
      return bys;
    }
    else {
      lock.unlock();
      return new Byte[Configure::GetMaxOverflowCache()];
    }
  }

  void CachePool::ReleaseOverflowCache(Byte* pBuf) {
    CachePool* pool = GetInstance();
    std::unique_lock<utils::SpinMutex> lock(pool->_spinMutexOvf);
    pool->_queueFreeOvf.push(pBuf);
  }

  Byte* CachePool::ApplyBys(uint32_t bufSize)
  {
    uint32_t sz = *_gSetBufSize.lower_bound(bufSize);
    return Apply(sz);
  }

  void CachePool::ReleaseBys(Byte* pBuf, uint32_t bufSize)
  {
    uint32_t sz = *_gSetBufSize.lower_bound(bufSize);
    return Release(pBuf, sz);
  }

  Byte* CachePool::Apply(uint32_t eleSize)
  {    
    CachePool* pool = GetInstance();
    auto iter = pool->_mapPool.find(eleSize);
    if (iter == pool->_mapPool.end())
    {
      std::unique_lock< utils::SpinMutex> lock(pool->_spinMutex);
      iter = pool->_mapPool.find(eleSize);
      if (iter == pool->_mapPool.end())
      {
        pool->_mapPool.insert(pair<uint32_t, BufferPool*>(eleSize, new BufferPool(eleSize)));
        iter = pool->_mapPool.find(eleSize);
      }
    }

    return iter->second->Apply();
  }

  void CachePool::Release(Byte* pBuf, uint32_t eleSize)
  {
    CachePool* pool = GetInstance();
    auto iter = pool->_mapPool.find(eleSize);
    assert(iter != pool->_mapPool.end());

    iter->second->Release(pBuf);
  }

  Buffer* CachePool::AllotBuffer(uint32_t eleLen)
  {
    CachePool* pool = GetInstance();
    std::unique_lock< utils::SpinMutex> lock(pool->_spinMutex);

    if (pool->_queueFreeBuf.size() > 0) 
    {
      Buffer* buf = GetInstance()->_queueFreeBuf.front();
      buf->Init(eleLen);
      GetInstance()->_queueFreeBuf.pop();
      return buf;
    }

    //if (pool->_szMemUsed >= Configure::GetTotalCacheSize())
    //{
    //  throw utils::ErrorMsg(CM_EXCEED_LIMIT, {});
    //}

    pool->_szMemUsed += Configure::GetCacheBlockSize();
    return new Buffer(eleLen);
  }
  
  void CachePool::RecycleBuffer(Buffer* buf)
  {
    CachePool* pool = GetInstance();
    std::unique_lock< utils::SpinMutex> lock(pool->_spinMutex);
    if (pool->_queueFreeBuf.size() > Configure::GetMaxFreeBufferCount())
    {
      delete buf;
    }
    else
    {
      pool->_queueFreeBuf.push(buf);
    }
  }

  CachePool::CachePool() : _szMemUsed(0), _mapPool(500)
  {

  }

  CachePool::~CachePool()
  {
    while (_queueFreeBuf.size() > 0)
    {
      free(_queueFreeBuf.front());
      _queueFreeBuf.pop();
    }
  }
}