#pragma once
#include "../cache/CachePool.h"
#include "../header.h"
#include "ActionType.h"
#include "IndexType.h"
#include <cstdint>

namespace storage {
static const uint16_t TWO_SHORT_LEN = 2 * sizeof(uint16_t);
static const uint16_t SHORT_LEN = 2;
static const uint16_t INT_LEN = 4;
static const uint16_t LLONG_LEN = 8;

class IndexPage;
class IndexTree;

class RawRecord {
public:
  RawRecord(IndexTree *indexTree, IndexPage *parentPage, Byte *bys, bool bSole)
      : _parentPage(parentPage), _bysVal(bys), _indexTree(indexTree),
        _bSole(bSole) {}
  RawRecord(const RawRecord &src) = delete;

  inline Byte *GetBysValue() const { return _bysVal; }
  /**Only the bytes' length in IndexPage, key length + value length without
   * overflow fields*/
  inline uint16_t GetTotalLength() const { return *((uint16_t *)_bysVal); }
  inline uint16_t GetKeyLength() const {
    return *((uint16_t *)(_bysVal + sizeof(uint16_t)));
  }
  inline void SetParentPage(IndexPage *page) { _parentPage = page; }
  inline IndexPage *GetParentPage() const { return _parentPage; }
  inline IndexTree *GetTreeFile() const { return _indexTree; }
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
  virtual ~RawRecord() {
    if (_bSole && _bysVal != nullptr)
      CachePool::Release(_bysVal, GetTotalLength());
  }

protected:
  /** the byte array that save key and value's content */
  Byte *_bysVal;
  /** the parent page included this record */
  IndexPage *_parentPage;
  /**index tree*/
  IndexTree *_indexTree;
  /**How many times this record is referenced*/
  atomic<int32_t> _refCount = 1;
  /**If this record' value is saved to solely buffer or branch page*/
  bool _bSole;
  /**Below variables only used for LeafRecord, put here to save 8 bytes memory*/
  /**Normal time its value is unknown, in transaction time, it is the actual
   * action*/
  ActionType _actionType = ActionType::UNKNOWN;
};
} // namespace storage
