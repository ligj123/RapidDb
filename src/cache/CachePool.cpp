#include "CachePool.h"
#include "../config/Configure.h"
#include "StackTrace.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef _MSVC_LANG
#include <malloc.h>
#include <stdio.h>
#else
#include <cstdio>
#include <cstdlib>
#endif // _MSVC_LANG

namespace storage {
uint16_t _arrSzMap[] = {16,   32,   48,   64,    96,    128,   160,
                        192,  224,  256,  384,   512,   640,   768,
                        896,  1024, 1536, 2048,  2560,  3072,  3584,
                        4096, 6144, 8192, 10240, 12288, 14336, 16384};

unordered_set<LocalMap *> CachePool::_setLocalMap;
thread_local LocalMap CachePool::_localMap;
CachePool *CachePool::_gCachePool = []() { return new CachePool; }();

#ifdef CACHE_TRACE
SpinMutex CachePool::_spinTrace;
unordered_map<uint64_t, string> CachePool::_mapApply;
bool CachePool::_bWriteLog{true};

fstream CreateStream() {
  if (!CachePool::_bWriteLog) {
    return fstream();
  }

  time_t sec = chrono::duration_cast<chrono::seconds>(
                   chrono::system_clock::now().time_since_epoch())
                   .count();
  stringstream ss;
  ss << put_time(gmtime(&sec), "%Y-%m-%d_%H_%M_%S.log");

  filesystem::path path("./st");
  if (!filesystem::exists(path)) {
    filesystem::create_directories(path);
  }

  path += "/" + ss.str();
  fstream fs(path.string(), ios_base::binary | ios_base::out | ios_base::app);
  bool b = fs.is_open();
  return fs;
}

fstream fs_stack(CreateStream());
#endif

LocalMap::LocalMap() {
  for (int i = 0; i < 28; i++) {
    _arrVctByte[i].reserve(_arrSzMap[i] >= 512 ? 32 : 16384 / _arrSzMap[i]);
  }

  unique_lock<SpinMutex> lock(CachePool::GetInstance()->_spinMutex);
  CachePool::_setLocalMap.insert(this);
}

LocalMap::~LocalMap() {
  for (int i = 0; i < 28; i++) {
    CachePool::BatchRelease(i, _arrVctByte[i], true);
  }

  unique_lock<SpinMutex> lock(CachePool::GetInstance()->_spinMutex);
  size_t rt = CachePool::_setLocalMap.erase(this);
  assert(rt == 1);

  CachePool::GetInstance()->_szMemused += _memStat;
  bStoped = true;
}

void LocalMap::Push(Byte *pBuf, uint16_t pos) {
  assert(pos >= 0 && pos < 28);
  // assert(!bStoped);

  vector<Byte *> &vctByte = _arrVctByte[pos];
  if (vctByte.size() >= vctByte.capacity()) {
    CachePool::BatchRelease(pos, vctByte, false);
  }

  vctByte.push_back(pBuf);
  _memStat -= _arrSzMap[pos];
}

Byte *LocalMap::Pop(uint16_t pos) {
  assert(pos >= 0 && pos < 28);

  vector<Byte *> &vctByte = _arrVctByte[pos];
  if (vctByte.size() == 0) {
    CachePool::BatchApply(pos, vctByte);
  }

  Byte *buf = vctByte.back();
  vctByte.pop_back();
  _memStat += _arrSzMap[pos];
  return buf;
}

CachePool::CachePool() {
  for (int i = 0; i < 28; i++) {
    _arrayPool[i] = new BufferPool(_arrSzMap[i]);
  }

  _vctFreeBuf.reserve(Configure::GetMaxFreeBufferCount());
  _vctFreeBlock.reserve(Configure::GetMaxFreeResultBlock());
}

CachePool::~CachePool() {
  for (int i = 0; i < 28; i++) {
    delete _arrayPool[i];
  }

  for (Buffer *buff : _vctFreeBuf) {
    delete buff;
  }

  for (Byte *bys : _vctFreeBlock) {
    std::free(bys);
  }
}

int64_t CachePool::GetMemoryAllocated() {
  CachePool *pool = GetInstance();
  unique_lock<SpinMutex> lock(pool->_spinMutex);
  return pool->_szMemAllocated +
         pool->_totalBlockNum * Configure::GetResultPageSize();
}

int64_t CachePool::GetMemoryUsed() {
  CachePool *pool = GetInstance();
  unique_lock<SpinMutex> lock(pool->_spinMutex);
  int64_t total =
      pool->_szMemused + pool->_totalBlockNum * Configure::GetResultPageSize();

  for (LocalMap *localMap : _setLocalMap) {
    total += localMap->GetMemStat();
  }

  return total;
}
void CachePool::BatchApply(uint32_t pos, vector<Byte *> &vct) {
  CachePool *pool = GetInstance();
  pool->_arrayPool[pos]->Apply(vct);
}

void CachePool::BatchRelease(uint32_t pos, vector<Byte *> &vct, bool bAll) {
  CachePool *pool = GetInstance();
  pool->_arrayPool[pos]->Release(vct, bAll);
}

Buffer *CachePool::AllocateBuffer(uint32_t eleLen) {
  CachePool *pool = GetInstance();
  std::unique_lock<SpinMutex> lock(pool->_spinMutex);

  if (pool->_vctFreeBuf.size() > 0) {
    Buffer *buf = pool->_vctFreeBuf.back();
    pool->_vctFreeBuf.pop_back();
    buf->Init(eleLen);
    return buf;
  } else {
    pool->_szMemAllocated += Configure::GetCacheBlockSize();
    return new Buffer(eleLen);
  }
}

void CachePool::RecycleBuffer(Buffer *buf) {
  CachePool *pool = GetInstance();
  std::unique_lock<SpinMutex> lock(pool->_spinMutex);
  if (pool->_vctFreeBuf.size() > Configure::GetMaxFreeBufferCount()) {
    delete buf;
    pool->_szMemAllocated -= Configure::GetCacheBlockSize();
  } else {
    pool->_vctFreeBuf.push_back(buf);
  }
}

#ifdef CACHE_TRACE
Byte *CachePool::ApplyBlock() {
  CachePool *pool = GetInstance();
  unique_lock<SpinMutex> lock(pool->_spinMutex);
  Byte *bys = nullptr;
  if (pool->_vctFreeBlock.size() > 0) {
    bys = pool->_vctFreeBlock.back();
    pool->_vctFreeBlock.pop_back();
  } else {
    bys = reinterpret_cast<Byte *>(std::malloc(Configure::GetResultPageSize()));
    pool->_totalBlockNum++;
  }

  lock.unlock();

  string str = StackTrace();
  unique_lock<SpinMutex> lock2(_spinTrace);
  _mapApply.emplace((uint64_t)bys, "ApplyBlock\n" + str);
  if (_bWriteLog) {
    fs_stack << (void *)bys << "\tApplyBlock\n" << str;
  }
  return bys;
}

void CachePool::ReleaseBlock(Byte *bys) {
  CachePool *pool = GetInstance();
  unique_lock<SpinMutex> lock(pool->_spinMutex);
  if (pool->_vctFreeBlock.size() > Configure::GetMaxFreeResultBlock()) {
    pool->_totalBlockNum--;
    std::free(bys);
  } else {
    pool->_vctFreeBlock.push_back(bys);
  }

  lock.unlock();
  unique_lock<SpinMutex> lock2(_spinTrace);
  size_t rt = _mapApply.erase((uint64_t)bys);
  assert(rt == 1);
  if (_bWriteLog) {
    fs_stack << (void *)bys << "\tReleaseBlock\n" << StackTrace();
  }
}

/**Apply a memory block for an index page*/
Byte *CachePool::ApplyPage() {
  Byte *bys = _localMap.Pop(PAGE_POS);

  string str = StackTrace();
  unique_lock<SpinMutex> lock(_spinTrace);
  _mapApply.emplace((uint64_t)bys, "ApplyPage\n" + str);
  if (_bWriteLog) {
    fs_stack << (void *)bys << "\tApplyPage\n" << str;
  }
  return bys;
}

/**Release a memory block for an index page*/
void CachePool::ReleasePage(Byte *page) {
  _localMap.Push(page, PAGE_POS);

  unique_lock<SpinMutex> lock(_spinTrace);
  size_t rt = _mapApply.erase((uint64_t)page);
  assert(rt == 1);
  if (_bWriteLog) {
    fs_stack << (void *)page << "\tRelease Page\n" << StackTrace();
  }
}

/**Apply a memory block from cache*/
Byte *CachePool::Apply(uint32_t bufSize) {
  assert(bufSize > 0);
  uint32_t pos = CalcBufSize(bufSize);
  if (pos == UINT32_MAX) {
    return MallocLargeBlock(bufSize);
  } else {
    Byte *bys = _localMap.Pop(pos);

    string str = StackTrace();
    unique_lock<SpinMutex> lock(_spinTrace);
    _mapApply.emplace((uint64_t)bys,
                      "Apply1 size: " + to_string(bufSize) + "  actual size: " +
                          to_string(_arrSzMap[pos]) + "\n" + str);
    if (_bWriteLog) {
      fs_stack << (void *)bys << "\tApply1 size: " << to_string(bufSize)
               << "  actual size: " << to_string(_arrSzMap[pos]) << "\n"
               << str;
    }

    return bys;
  }
}
/**Apply a memory block from cache and set the actual allocated size*/
Byte *CachePool::Apply(uint32_t bufSize, uint32_t &realSize) {
  assert(bufSize > 0);
  uint32_t pos = CalcBufSize(bufSize);
  if (pos == UINT32_MAX) {
    realSize = bufSize;
    return MallocLargeBlock(bufSize);
  } else {
    Byte *bys = _localMap.Pop(pos);
    realSize = _arrSzMap[pos];

    string str = StackTrace();
    unique_lock<SpinMutex> lock(_spinTrace);
    _mapApply.emplace((uint64_t)bys,
                      "Apply2:  size: " + to_string(bufSize) +
                          "  actual size: " + to_string(realSize) + "\n" + str);
    if (_bWriteLog) {
      fs_stack << (void *)bys << "\tApply2 size: " << to_string(bufSize)
               << "  actual size: " << to_string(realSize) << "\n"
               << str;
    }
    return bys;
  }
}
/**Release a memory block with unfixed size*/
void CachePool::Release(Byte *pBuf, uint32_t bufSize) {
  uint32_t pos = CalcBufSize(bufSize);
  if (pos == UINT32_MAX) {
    FreeLargeBlock(pBuf, bufSize);
  } else {
    _localMap.Push(pBuf, pos);

    unique_lock<SpinMutex> lock(_spinTrace);
    size_t rt = _mapApply.erase((uint64_t)pBuf);
    assert(rt == 1);
    if (_bWriteLog) {
      fs_stack << (void *)pBuf << "\tRelease size: " << to_string(bufSize)
               << "\n"
               << StackTrace();
    }
  }
}
#endif // CACHE_TRACE

Byte *CachePool::MallocLargeBlock(uint32_t bufsize) {
  if ((bufsize & 0x3FFF) == 0) {
#ifdef _MSVC_LANG
    return (Byte *)_aligned_malloc(bufsize, 4096);
#else
    return (Byte *)aligned_alloc(4096, bufsize);
#endif // _MSVC_LANG
  } else {
    return new Byte[bufsize];
  }
}

void CachePool::FreeLargeBlock(Byte *buf, uint32_t bufsize) {
  if ((bufsize & 0xFFF) == 0) {
#ifdef _MSVC_LANG
    _aligned_free(buf);
#else
    free(buf);
#endif // _MSVC_LANG
  } else {
    delete[] buf;
  }
}
} // namespace storage
