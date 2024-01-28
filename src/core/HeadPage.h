#pragma once
#include "../cache/Mallocator.h"
#include "../config/FileVersion.h"
#include "CachePage.h"
#include "IndexType.h"
#include <map>

namespace storage {

struct KeyCmp {
  bool operator()(const uint64_t lhs, const uint64_t rhs) const {
    return !(lhs <= rhs);
  }
};

/**The stamps only valid for primary index*/
class HeadPage : public CachePage {
public:
  /**The max versions can be support at the same time in a table*/
  static const uint16_t MAX_RECORD_VERSION_COUNT;

  /**Offset to save page type*/
  static const uint16_t PAGE_TYPE_OFFSET;
  /**Offfset tosave index type*/
  static const uint16_t INDEX_TYPE_OFFSET;
  /**Offset to save how many versions for record can saved to this table. For
   * single version, it will always be 1*/
  static const uint16_t RECORD_VERSION_COUNT_OFFSET;
  /**Offset to save this file version*/
  static const uint16_t FILE_VERSION_OFFSET;
  /**The offset to save how many alterable columns in this key*/
  static const uint16_t KEY_ALTERABLE_FIELD_COUNT_OFFSET;
  /**The offset to save how many alterable columns in this value*/
  static const uint16_t VALUE_ALTERABLE_FIELD_COUNT_OFFSET;
  /**The offset the first page to save garbage page id, this page and the
   * following pages used to save all garbage pages ids and length.*/
  static const uint16_t GARBAGE_PAGE_OFFSET;
  /**How many series pages have been used to save the garbage page ids*/
  static const uint16_t GARBAGE_PAGES_NUM_OFFSET;
  /**There have how many garbage items in total, every item include the first
   * page id and the series page num*/
  static const uint16_t GRABAGE_TOTAL_ITEMS_OFFSET;
  /**To Save the crc32 for garbage pages*/
  static const uint16_t GARBAGE_CRC32_OFFSET;

  /**The offset to save the total page count in this index*/
  static const uint16_t TOTAL_PAGES_COUNT_OFFSET;
  /**The offset to save the root page id*/
  static const uint16_t ROOT_PAGE_OFFSET;
  /**The offset to save the begin leaf page id*/
  static const uint16_t BEGIN_LEAF_PAGE_OFFSET;
  /**The offset to save the end leaf page id*/
  static const uint16_t END_LEAF_PAGE_OFFSET;
  /**The offset to save how many records in this index*/
  static const uint16_t TOTAL_RECORD_COUNT_OFFSET;
  /**The offset to save auto increment key*/
  static const uint16_t AUTO_INCREMENT_KEY_OFFSET;
  /**The offset to save current record stamp for new record*/
  static const uint16_t CURRENT_RECORD_STAMP_OFFSET;
  /**The offset to save the version's stamps and time for this table*/
  static const uint16_t RECORD_VERSION_STAMP_OFFSET;

protected:
  /**How many length alterable columns in key*/
  uint16_t _keyAlterableFieldCount = 0;
  /**How many length alterable columns in value*/
  uint16_t _valueAlterableFieldCount = 0;
  /**The stamp versions that this table support in current time*/
  uint8_t _recordVerCount = 0;
  /**Index type, primary, unique or non-unique*/
  IndexType _indexType = IndexType::PRIMARY;
  /**the count of total pages in this index*/
  uint32_t _totalPageCount{0};
  /**The root page id for this index*/
  PageID _rootPageId{PAGE_NULL_POINTER};
  /**The begin leaf page for this index*/
  PageID _beginLeafPageId{PAGE_NULL_POINTER};
  /**The end leaf page for this index*/
  PageID _endLeafPageId{PAGE_NULL_POINTER};
  /**The count of total records in this index*/
  uint64_t _totalRecordCount{0};
  /**To generate the auto increment ids, the value is current id*/
  uint64_t _autoIncrementKey{0};
  // In this table, any changes for a record will need a new record version
  // stamp, it start from 0, and will add one every time. This stamp will be
  // used for log transport, data snapshot etc.
  uint64_t _currRecordStamp{0};

/**The map contains the pair of timestamp and record stamps in this table,
 * only valid for primary index in a table. It saved in head page after
 * RECORD_VERSION_STAMP_OFFSET and save the pairs from small to big one by one
 */
#ifndef SINGLE_VERSION
  MTreeMap<DT_MicroSec, VersionStamp, KeyCmp> _mapVerStamp;
  MTreeSet<VersionStamp, KeyCmp> _setVerStamp;
#endif

public:
  HeadPage(IndexTree *indexTree)
      : CachePage(indexTree, UINT32_MAX, PageType::HEAD_PAGE) {}
  void InitHeadPage(IndexType iType, const VectorDataValue &vctVal);
  bool SaveToBuffer() override;
  void LoadVars() override;
  uint32_t PageSize() const override { return HEAD_PAGE_SIZE; }

  void WriteFileVersion();
  FileVersion ReadFileVersion();

#ifndef SINGLE_VERSION
  inline Byte ReadRecordVersionCount() { return (Byte)_mapVerStamp.size(); }
  // Only after the entire table was locked, here can update record version. so
  // here do not need lock
  inline void AddNewRecordVersion(VersionStamp ver, DT_MicroSec timeStamp) {
    assert(_mapVerStamp.size() < MAX_RECORD_VERSION_COUNT);
    if (_mapVerStamp.size() > 0) {
      auto iter = _mapVerStamp.rbegin();
      assert(timeStamp > iter->first && ver > iter->second);
    }

    _mapVerStamp.insert({timeStamp, ver});
    _setVerStamp.insert(ver);
    _bHeadChanged = true;
  }
  /**Now only support input the time of version. It maybe change in following
   * time*/
  inline VersionStamp GetRecordVersionStamp(DT_MicroSec timeStamp) {
    auto iter = _mapVerStamp.find(timeStamp);
    assert(iter != _mapVerStamp.end());
    return iter->second;
  }

  inline void RemoveRecordVersionStamp(Byte ver) {
    assert(ver < _mapVerStamp.size());
    for (auto iter = _mapVerStamp.begin(); iter != _mapVerStamp.end(); iter++) {
      ver--;
      if (ver == 0) {
        _setVerStamp.erase(iter->second);

        _mapVerStamp.erase(iter);
        break;
      }
    }
  }

  inline const MTreeSet<VersionStamp, KeyCmp> &GetSetVerStamp() {
    return _setVerStamp;
  }

  inline VersionStamp GetLastVersionStamp() {
    if (_mapVerStamp.size() == 0)
      return 0;
    return _mapVerStamp.rbegin()->second;
  }
#endif
  // Apply one or more stamps. If the index tree only has one task to execute,
  // every time apply one stamp. If there has too much statements and the index
  // tree split several task to execute, every task will apply a range of
  // stamps every time and use atomic_ref to ensure the consistency.
  inline VersionStamp GetAndIncRecordStamp(uint64_t num = 1) {
    if (num == 1) {
      return _currRecordStamp++;
    } else {
      return atomic_ref<VersionStamp>(_currRecordStamp)
          .fetch_add(num, memory_order_relaxed);
    }

    _bDirty = true;
  }

  inline VersionStamp ReadRecordStamp() { return _currRecordStamp; }

  void WriteRecordStamp(VersionStamp recordStamp) {
    _currRecordStamp = recordStamp;
    _bDirty = true;
  }

  inline PageID GetAndIncTotalPageCount(uint32_t pageNum = 1,
                                        bool atm = false) {
    if (!atm) {
      PageID id = _totalPageCount;
      _totalPageCount += pageNum;
      return id;
    } else {
      return atomic_ref<uint32_t>(_totalPageCount)
          .fetch_add(pageNum, memory_order_relaxed);
    }
    _bDirty = true;
  }

  inline uint32_t ReadTotalPageCount() { return _totalPageCount; }

  void WriteTotalPageCount(uint32_t totalPage) {
    _totalPageCount = totalPage;
    _bDirty = true;
  }

  inline uint64_t ReadTotalRecordCount() { return _totalRecordCount; }

  inline void WriteTotalRecordCount(uint64_t totalRecNum) {
    _totalRecordCount = totalRecNum;
    _bDirty = true;
  }

  inline uint64_t IncAndGetTotalRecordCount(uint64_t recNum = 1,
                                            bool atm = false) {
    if (atm) {
      uint64_t val = atomic_ref<uint64_t>(_totalRecordCount)
                         .fetch_add(recNum, memory_order_relaxed);
      return val + recNum;
    } else {
      _totalRecordCount += recNum;
      return _totalRecordCount;
    }
  }

  inline void WriteRootPagePointer(uint32_t pointer) {
    _rootPageId = pointer;
    _bDirty = true;
  }

  inline uint32_t ReadRootPagePointer() { return _rootPageId; }

  inline void WriteBeginLeafPagePointer(uint32_t pointer) {
    _beginLeafPageId = pointer;
    _bDirty = true;
  }

  inline uint32_t ReadBeginLeafPagePointer() { return _beginLeafPageId; }

  inline void WriteEndLeafPagePointer(uint32_t pointer) {
    _endLeafPageId = pointer;
    _bDirty = true;
  }

  inline uint64_t ReadEndLeafPagePointer() { return _endLeafPageId; }

  inline IndexType ReadIndexType() { return _indexType; }

  // Apply one or more auto increment key. If the index tree only has one task
  // to execute, every time apply one key. If there has too much statements and
  // the index tree split several task to execute, every task will apply a range
  // of keys every time and use atomic_ref to ensure the consistency.
  inline uint64_t GetAndAddAutoIncrementKey(uint64_t step = 1) {
    if (step == 1) {
      _autoIncrementKey++;
      return _autoIncrementKey;
    } else {
      return atomic_ref<uint64_t>(_autoIncrementKey)
          .fetch_add(step, memory_order_relaxed);
    }
    _bDirty = true;
  }

  inline uint64_t ReadAutoIncrementKey() { return _autoIncrementKey; }

  inline void WriteAutoIncrementKey(uint64_t key) {
    _autoIncrementKey = key;
    _bDirty = true;
  }

  inline uint16_t ReadKeyVariableFieldCount() {
    return _keyAlterableFieldCount;
  }

  inline uint16_t ReadValueVariableFieldCount() {
    return _valueAlterableFieldCount;
  }

  void ReadGarbage(uint32_t &totalPage, PageID &pageId, uint16_t &usedPage,
                   uint32_t &crc32) {
    pageId = ReadInt(GARBAGE_PAGE_OFFSET);
    usedPage = ReadShort(GARBAGE_PAGES_NUM_OFFSET);
    totalPage = ReadInt(GRABAGE_TOTAL_ITEMS_OFFSET);
    crc32 = ReadInt(GARBAGE_CRC32_OFFSET);
  }

  void WriteGabage(uint32_t totalPage, PageID pageId, int16_t usedPage,
                   uint32_t crc32) {
    WriteInt(GARBAGE_PAGE_OFFSET, pageId);
    WriteShort(GARBAGE_PAGES_NUM_OFFSET, usedPage);
    WriteInt(GRABAGE_TOTAL_ITEMS_OFFSET, totalPage);
    WriteInt(GARBAGE_CRC32_OFFSET, crc32);
    _bDirty = true;
  }
};
} // namespace storage
