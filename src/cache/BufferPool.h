#pragma once
#include "../config/Configure.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include <stack>
#include <unordered_map>
#include <vector>

namespace storage {
using namespace std;
class Buffer {
public:
  inline static Byte *CalcAddr(Byte *sAddr) {
    return sAddr - ((uint64_t)sAddr & (Configure::GetCacheBlockSize() - 1));
  }

public:
  Buffer(uint32_t eleSize);
  ~Buffer();
  Byte *Apply();
  void Release(Byte *bys);
  void Apply(vector<Byte *> &vct);
  void Release(vector<Byte *> &vct, bool bAll);
  void Init(uint32_t eleSize);
  inline bool IsEmpty() { return _stackFree.size() == 0; }
  inline bool IsFull() { return _stackFree.size() == _maxEle; }
  inline Byte *GetBuf() { return _pBuf; }

protected:
  Byte *_pBuf;
  uint32_t _eleSize;
  uint32_t _maxEle;
  stack<uint32_t> _stackFree;
};

class BufferPool {
public:
  BufferPool(uint32_t eleSize);
  ~BufferPool();
  void Apply(vector<Byte *> &vct);
  void Release(vector<Byte *> &vct, bool bAll);
  Byte *Apply();
  void Release(Byte *bys);

protected:
  unordered_map<Byte *, Buffer *> _mapBuffer;
  unordered_map<Byte *, Buffer *> _mapFreeBuffer;
  uint32_t _eleSize;
  utils::SpinMutex _spinMutex;
};
} // namespace storage
