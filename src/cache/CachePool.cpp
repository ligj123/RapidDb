#include "CachePool.h"
#include "../utils/ErrorMsg.h"
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include <string>

namespace storage {
  CachePool* CachePool::_gCachePool = []() {
    return new CachePool;
  }();

  Byte* CachePool::Apply(uint32_t eleSize)
  {    
    CachePool* pool = GetInstance();
    auto iter = pool->_mapPool.find(eleSize);
    if (iter == pool->_mapPool.end())
    {
      pool->_spinMutex.lock();
      iter = pool->_mapPool.find(eleSize);
      if (iter == pool->_mapPool.end())
      {
        pool->_mapPool.insert(pair<uint32_t, BufferPool*>(eleSize, new BufferPool(eleSize)));
        iter = pool->_mapPool.find(eleSize);
      }

      pool->_spinMutex.unlock();
    }

    return iter->second->Apply();
  }

  void CachePool::Release(Byte* pBuf, uint32_t eleSize)
  {
    CachePool* pool = GetInstance();
    auto iter = pool->_mapPool.find(eleSize);
    if (iter == pool->_mapPool.end())
    {
      throw utils::ErrorMsg(CM_FAILED_FIND_SIZE, { std::to_string(eleSize) });
    }

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

    if (pool->_szMemUsed >= Configure::GetTotalCacheSize())
    {
      throw utils::ErrorMsg(CM_EXCEED_LIMIT, {});
    }

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