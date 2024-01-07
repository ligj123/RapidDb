#pragma once
#include "../cache/CachePool.h"
#include "../header.h"
#include "ActionType.h"
#include "IndexType.h"
#include <cstdint>

namespace storage {
class IndexPage;
class IndexTree;

class RawRecord {
public:
  RawRecord(Byte *bys, bool bSole, IndexType type)
      : _bysVal(bys), _bSole(bSole), _indexType(type) {}
  RawRecord(RawRecord &&src)
      : _bysVal(src._bysVal), _bSole(src._bSole), _indexType(src._indexType) {
    src._bysVal = nullptr;
  }
  virtual ~RawRecord() {
    if (_bSole && _bysVal != nullptr)
      CachePool::Release(_bysVal, *((uint16_t *)_bysVal));
  }

  inline Byte *GetBysValue() const { return _bysVal; }

  inline uint16_t GetKeyLength() const {
    return *((uint16_t *)(_bysVal + UI16_LEN));
  }
  virtual uint16_t GetTotalLength() const = 0;
  virtual uint16_t GetValueLength() const = 0;
  virtual bool IsSole() const { return _bSole; }
  virtual bool IsTransaction() const { return false; }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  /** the byte array that save key and value's content */
  Byte *_bysVal;
  /**If this record' value is saved into solely buffer or into index page*/
  bool _bSole;
  /**IndexType*/
  IndexType _indexType;
};
} // namespace storage
