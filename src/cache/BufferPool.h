#pragma once
#include "../config/Configure.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include <unordered_map>
#include <vector>

namespace storage {
using namespace std;
const uint64_t BUFFER_MASK = Configure::GetCacheBlockSize() - 1;
class Buffer {
public:
  inline static Byte *CalcAddr(Byte *sAddr) {
    return sAddr - ((uint64_t)sAddr & BUFFER_MASK);
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
#ifdef _DEBUG_TEST
  size_t UsedMem() { return _eleSize * (_maxEle - _vctFree.size()); }
#endif // _DEBUG_TEST
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
#ifdef _DEBUG_TEST
  size_t UsedMem() {
    size_t sz = 0;
    for (auto iter = _mapBuffer.begin(); iter != _mapBuffer.end(); iter++) {
      sz += iter->second->UsedMem();
    }
    return sz;
  }
#endif // _DEBUG_TEST
protected:
  unordered_map<Byte *, Buffer *> _mapBuffer;
  unordered_map<Byte *, Buffer *> _mapFreeBuffer;
  uint16_t _eleSize;
  SpinMutex _spinMutex;
};
} // namespace storage
