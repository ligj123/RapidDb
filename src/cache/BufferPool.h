#pragma once
#include "../header.h"
#include <queue>
#include <unordered_map>
#include "../utils/SpinMutex.h"
#include "../config/Configure.h"

namespace storage {
  using namespace std;
  class Buffer
  {
  public:
    inline static Byte* CalcAddr(Byte* sAddr) {
      return sAddr - ((uint64_t)sAddr & (Configure::GetCacheBlockSize() - 1));
    }

  public:
    Buffer(uint32_t eleSize);
    ~Buffer();
    Byte* Apply();
    void Release(Byte* bys);
    void Init(uint32_t eleSize);
    inline bool IsEmpty() { return (_idxTail - _idxHead) == _maxEle; }
    inline bool IsFull() { return (_idxTail - _idxHead) == 0; }
    inline Byte* GetBuf() { return _pBuf; }
    //inline uint16_t CalcPos(Byte* sAddr)
    //{
    //  return (uint16_t)(((uint64_t)sAddr & (Configure::GetCacheBlockSize() - 1)) / _eleSize);
    //}

  protected:
    Byte* _pBuf;
    uint32_t _eleSize;
    uint32_t _maxEle;
    uint32_t _idxHead;
    uint32_t _idxTail;

    //queue<uint16_t> _queueFree;
  };

  class BufferPool
  {
  public:
    BufferPool(uint32_t eleSize);
    ~BufferPool();
    Byte* Apply();
    void Release(Byte* bys);
  protected:
    unordered_map<Byte*, Buffer*> _mapBuffer;
    unordered_map<Byte*, Buffer*> _mapFreeBuffer;
    uint32_t _eleSize;
    utils::SpinMutex _spinMutex;
  };
}