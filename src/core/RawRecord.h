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
  RawRecord(IndexTree *indexTree, IndexPage *parentPage, Byte *bys, bool bSole)
      : _parentPage(parentPage), _bysVal(bys), _indexTree(indexTree),
        _bSole(bSole) {}
  RawRecord(RawRecord &src)
      : _bysVal(src._bysVal), _parentPage(src._parentPage),
        _indexTree(src._indexTree), _bSole(src._bSole),
        _actionType(src._actionType), _gapLock(src._gapLock) {
    src._bysVal = nullptr;
  }
  virtual ~RawRecord() {
    if (_bSole && _bysVal != nullptr)
      CachePool::Release(_bysVal, *((uint16_t *)_bysVal));
  }

  inline Byte *GetBysValue() const { return _bysVal; }
  /**Only the bytes' length in IndexPage, key length + value length without
   * overflow page content*/
  inline uint16_t GetTotalLength() const {
    return _bRemoved ? 0 : *((uint16_t *)_bysVal);
  }
  inline uint16_t GetKeyLength() const {
    return *((uint16_t *)(_bysVal + UI16_LEN));
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
  /** the byte array that save key and value's content */
  Byte *_bysVal;
  // Only one is valid, if just create a record and not added into a page,
  // _indexTree is valid, if has added into a page, _parentPage is valid.
  union {
    /** the parent page included this record */
    IndexPage *_parentPage;
    /**index tree*/
    IndexTree *_indexTree;
  };
  // True: _parentPage is valid, False: _indexTree is valid
  bool _InPage;
  /**If this record' value is saved into solely buffer or into index page*/
  bool _bSole;
};
} // namespace storage
