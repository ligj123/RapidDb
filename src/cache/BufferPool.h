#pragma once
#include "../header.h"
#include <set>
#include <map>
#include "../utils/SpinMutex.h"
#include "../config/Configure.h"

namespace storage {
  using namespace std;
  class Buffer
  {
  public:
    static Byte* CalcAddr(Byte* sAddr)
    {
      return sAddr - ((uint64_t)sAddr & (Configure::GetCacheBlockSize() - 1));
    }

  public:
    Buffer(uint32_t eleSize);
    ~Buffer();
    Byte* Apply();
    void Release(Byte* bys);
    void Init(uint32_t eleSize);
    bool IsEmpty() { return _setFree.size() == _maxEle; }
    bool IsFull() { return _setFree.size() == 0; }
    operator Byte*() { return _pBuf; }
    inline uint16_t CalcPos(Byte* sAddr)
    {
      return (uint16_t)(((uint64_t)sAddr & (Configure::GetCacheBlockSize() - 1)) / _eleSize);
    }

  protected:
    Byte* _pBuf;
    uint32_t _eleSize;
    uint32_t _maxEle;
    set<uint16_t> _setFree;
  };

  class BufferPool
  {
  public:
    BufferPool(uint32_t eleSize);
    ~BufferPool();
    Byte* Apply();
    void Release(Byte* bys);
  protected:
    map<Byte*, Buffer*> _mapBuffer;
    map<Byte*, Buffer*> _mapFreeBuffer;
    uint32_t _eleSize;
    utils::SpinMutex _spinMutex;
  };
}