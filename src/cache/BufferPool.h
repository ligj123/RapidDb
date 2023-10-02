#pragma once
#include "../header.h"
#include "../utils/SpinMutex.h"
#include <unordered_map>
#include <vector>

namespace storage {
using namespace std;
class Buffer {
public:
  static const uint64_t BUFFER_MASK;
  static const uint64_t BUFFER_MASK_N;

public:
  inline static Byte *CalcAddr(Byte *sAddr) {
    return (Byte *)(((uint64_t)sAddr) & BUFFER_MASK_N);
  }

public:
  Buffer(uint16_t eleSize);
  ~Buffer();
  Byte *Apply();
  void Release(Byte *bys);
  void Apply(vector<Byte *> &vct);
  void Release(vector<Byte *> &vct, bool bAll);
  void Init(uint16_t eleSize);
  inline bool IsEmpty() { return _vctFree.size() == 0; }
  inline bool IsFull() { return _vctFree.size() == _maxEle; }
  inline Byte *GetBuf() { return _pBuf; }
#ifdef CACHE_TRACE
  size_t UsedMem() { return _eleSize * (_maxEle - _vctFree.size()); }
#endif // CACHE_TRACE
protected:
  Byte *_pBuf;
  uint16_t _eleSize;
  uint16_t _maxEle;
  vector<uint16_t> _vctFree;
};

class BufferPool {
public:
  BufferPool(uint16_t eleSize);
  ~BufferPool();
  void Apply(vector<Byte *> &vct);
  void Release(vector<Byte *> &vct, bool bAll);
  Byte *Apply();
  void Release(Byte *bys);
#ifdef CACHE_TRACE
  size_t UsedMem() {
    size_t sz = 0;
    for (auto iter = _mapBuffer.begin(); iter != _mapBuffer.end(); iter++) {
      sz += iter->second->UsedMem();
    }
    return sz;
  }
#endif // CACHE_TRACE
protected:
  unordered_map<Byte *, Buffer *> _mapBuffer;
  unordered_map<Byte *, Buffer *> _mapFreeBuffer;
  uint16_t _eleSize;
  SpinMutex _spinMutex;
};
} // namespace storage
