#pragma once
#include "../cache/CachePool.h"
#include "../header.h"
#include "CoreEnum.h"

#include <cstdint>

namespace storage {
class IndexPage;
class IndexTree;

class RawRecord {
public:
  RawRecord(Byte *bys, bool bSole, IndexType type)
      : _bysVal(bys), _bSole(bSole), _indexType(type) {}
  RawRecord()
      : _bysVal(nullptr), _bSole(false), _indexType(IndexType::UNKNOWN) {}
  RawRecord(RawRecord &&src)
      : _bysVal(src._bysVal), _bSole(src._bSole), _indexType(src._indexType),
        _bDeleted(src._bDeleted) {
    src._bysVal = nullptr;
  }
  RawRecord(const RawRecord &src) = delete;
  RawRecord &operator=(RawRecord &&src) {
    _bysVal = src._bysVal;
    _bSole = src._bSole;
    _indexType = src._indexType;
    src._bysVal = nullptr;
    return *this;
  }

  RawRecord &operator=(const RawRecord &src) = delete;
  virtual ~RawRecord() {
    if (_bSole && _bysVal != nullptr)
      CachePool::Release(_bysVal, *((uint16_t *)_bysVal));
  }

  inline void UpdateBysValue(Byte *bys, bool sole = false) {
    if (_bSole && _bysVal != nullptr)
      CachePool::Release(_bysVal, *((uint16_t *)_bysVal));

    _bysVal = bys;
    _bSole = sole;
  }
  inline Byte *GetBysValue() const { return _bysVal; }
  inline uint16_t GetKeyLength() const {
    return *((uint16_t *)(_bysVal + UI16_LEN));
  }
  bool IsSole() const { return _bSole; }
  bool IsNull() { return _bysVal == nullptr; }
  virtual uint16_t GetTotalLength() const = 0;
  virtual uint16_t GetValueLength() const = 0;
  virtual bool IsInTransaction() const { return false; }

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
  /**To mark the record as delete when ActionType::DELETE, Only used in
   * LeafRecord*/
  bool _bDeleted{false};
  // The record is valid or not. If the key is exceed the length limit, it will
  // set to invliad and set exception when construct the record.
  bool _bValid{true};
};
} // namespace storage
