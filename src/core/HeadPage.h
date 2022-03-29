#pragma once
#include "../cache/Mallocator.h"
#include "../config/FileVersion.h"
#include "CachePage.h"
#include "IndexType.h"
#include <map>

namespace storage {
/**The stamps only valid for primary index*/
class HeadPage : public CachePage {
public:
  static const uint32_t HEAD_PAGE_LENGTH;
  /**The max versions can be support at the same time in a table*/
  static const uint16_t MAX_RECORD_VERSION_COUNT;
  /**Invalid page id = UINT32_MAX*/
  static const uint32_t PAGE_NULL_POINTER;

  /**Offset to save page type*/
  static const uint16_t PAGE_TYPE_OFFSET;
  /**Offfset tosave index type*/
  static const uint16_t INDEX_TYPE_OFFSET;
  /**Offset to save how many versions for record can saved to this table*/
  static const uint16_t RECORD_VERSION_COUNT_OFFSET;
  /**Offset to save this file version*/
  static const uint16_t VERSION_OFFSET;
  /**The offset to save how many length alterable columns in this key*/
  static const uint16_t KEY_ALTERABLE_FIELD_COUNT_OFFSET;
  /**The offset to save how many length alterable columns in this value*/
  static const uint16_t VALUE_ALTERABLE_FIELD_COUNT_OFFSET;
  /**The offset the first page to save garbage page id, this page and the
   * following pages used to save all garbage pages ids.*/
  static const uint16_t GARBAGE_PAGE_OFFSET;
  /**How many series pages have been used to save the garbage page ids*/
  static const uint16_t GARBAGE_SAVE_PAGES_NUM_OFFSET;
  /**There have how many garbage in total*/
  static const uint16_t GRABAGE_TOTAL_PAGES_OFFSET;

  /**The offset to save the total page count by this index*/
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
  static const uint16_t AUTO_INCREMENT_KEY;
  /**The offset to save auto increment key2*/
  static const uint16_t AUTO_INCREMENT_KEY2;
  /**The offset to save auto increment key3*/
  static const uint16_t AUTO_INCREMENT_KEY3;
  /**The offset to save current record stamp for new record*/
  static const uint16_t CURRENT_RECORD_STAMP_OFFSET;
  /**The offset to save the version's stamps and time for this table*/
  static const uint16_t RECORD_VERSION_STAMP_OFFSET;

protected:
  /**How many length alterable columns in key*/
  uint16_t _keyAlterableFieldCount = 0;
  /**How many length alterable columns in value*/
  uint16_t _valueAlterableFieldCount = 0;
  /**The versions that this table support in current time*/
  // uint8_t _recordVerCount = 0;
  /**Index type, primary, unique or non-unique*/
  IndexType _indexType = IndexType::PRIMARY;
  uint32_t _garbagePageId = UINT32_MAX;
  /**the count of total pages in this index*/
  atomic<uint32_t> _totalPageCount = 0;
  /**The root page id for this index*/
  atomic<PageID> _rootPageId = 0;
  /**The begin leaf page for this index*/
  atomic<PageID> _beginLeafPageId = 0;
  /**The end leaf page for this index*/
  atomic<PageID> _endLeafPageId = 0;
  /**The count of total records in this index*/
  atomic_uint64_t _totalRecordCount = 0;
  /**In this table, any changes for a record will need a new record version
  stamp, it start from 0, and will add one every time. This stamp will be used
  for log transport, data snapshot etc.*/
  atomic_uint64_t _currRecordStamp = 0;
  /**To generate the auto increment ids, the value is current id*/
  atomic_uint64_t _autoIncrementKey1 = 0;
  atomic_uint64_t _autoIncrementKey2 = 0;
  atomic_uint64_t _autoIncrementKey3 = 0;
  /**The map contains the pair of timestamp and record stamps in this table,
   * only valid for primary index in a table. It saved in head page after
   * RECORD_VERSION_STAMP_OFFSET and save the pairs from small to big one by one
   */
  map<uint64_t, uint64_t> _mapVerStamp;
  SpinMutex _spinMutex;

public:
  HeadPage(IndexTree *indexTree) : CachePage(indexTree, UINT32_MAX) {}

  void ReadPage() override;
  void WritePage() override;
  void WriteFileVersion();
  FileVersion ReadFileVersion();

  inline Byte ReadRecordVersionCount() { return (Byte)_mapVerStamp.size(); }

  inline void AddNewRecordVersion(uint64_t timeStamp) {
    lock_guard<SpinMutex> lock(_spinMutex);
    assert(_mapVerStamp.size() < MAX_RECORD_VERSION_COUNT);
    uint64_t ver = _currRecordStamp.load(memory_order_relaxed);
    if (_mapVerStamp.size() > 0) {
      auto iter = _mapVerStamp.rbegin();
      assert(timeStamp > iter->first && ver > iter->second);
    }

    _mapVerStamp.insert({timeStamp, ver});
  }
  /**Now only support input the time of version. It maybe change in following
   * time*/
  inline uint64_t GetRecordVersionStamp(uint64_t tm) {
    auto iter = _mapVerStamp.find(tm);
    assert(iter != _mapVerStamp.end());
    return iter->second;
  }

  inline void RemoveRecordVersionStamp(Byte ver) {
    lock_guard<SpinMutex> lock(_spinMutex);
    assert(ver < _mapVerStamp.size());
    for (auto iter = _mapVerStamp.begin(); iter != _mapVerStamp.end(); iter++) {
      ver--;
      if (ver == 0) {
        _mapVerStamp.erase(iter);
        break;
      }
    }
  }

  inline uint64_t GetLastVersionStamp() {
    if (_mapVerStamp.size() == 0)
      return 0;
    return _mapVerStamp.rbegin()->second;
  }

  inline uint64_t GetAndIncRecordStamp() {
    return _currRecordStamp.fetch_add(1, memory_order_relaxed);
  }

  inline uint64_t ReadRecordStamp() {
    return _currRecordStamp.load(memory_order_relaxed);
  }

  void WriteRecordStamp(uint64_t recordStamp) {
    _currRecordStamp.store(recordStamp, memory_order_relaxed);
  }

  inline uint32_t GetAndIncTotalPageCount(uint32_t pageNum = 1) {
    return _totalPageCount.fetch_add(pageNum, memory_order_relaxed);
  }

  inline uint32_t ReadTotalPageCount() {
    return _totalPageCount.load(memory_order_relaxed);
  }

  void WriteTotalPageCount(uint32_t totalPage) {
    _totalPageCount.store(totalPage, memory_order_relaxed);
  }

  inline uint64_t ReadTotalRecordCount() {
    return _totalRecordCount.load(memory_order_relaxed);
  }

  inline void WriteTotalRecordCount(uint64_t totalRecNum) {
    _totalRecordCount.store(totalRecNum, memory_order_relaxed);
  }

  inline uint64_t GetAndIncTotalRecordCount() {
    return _totalRecordCount.fetch_add(1, memory_order_relaxed);
  }

  inline void WriteRootPagePointer(uint32_t pointer) {
    _rootPageId.store(pointer, memory_order_relaxed);
  }

  inline uint32_t ReadRootPagePointer() {
    return _rootPageId.load(memory_order_relaxed);
  }

  inline void WriteBeginLeafPagePointer(uint32_t pointer) {
    _beginLeafPageId.store(pointer, memory_order_relaxed);
  }

  inline uint32_t ReadBeginLeafPagePointer() {
    return _beginLeafPageId.load(memory_order_relaxed);
  }

  inline void WriteEndLeafPagePointer(uint32_t pointer) {
    _endLeafPageId.store(pointer, memory_order_relaxed);
  }

  inline uint64_t ReadEndLeafPagePointer() {
    return _endLeafPageId.load(memory_order_relaxed);
  }

  inline void WriteIndexType(IndexType type) {
    lock_guard<SpinMutex> lock(_spinMutex);
    _indexType = type;
    WriteByte(INDEX_TYPE_OFFSET, (Byte)type);
  }

  inline IndexType ReadIndexType() { return _indexType; }

  inline uint64_t GetAndAddAutoIncrementKey(uint64_t step) {
    return _autoIncrementKey1.fetch_add(step, memory_order_relaxed);
  }

  inline uint64_t ReadAutoIncrementKey() {
    return _autoIncrementKey1.load(memory_order_relaxed);
  }

  inline void WriteAutoIncrementKey(uint64_t key) {
    _autoIncrementKey1.store(key, memory_order_relaxed);
  }

  inline uint64_t GetAndAddAutoIncrementKey2(uint64_t step) {
    return _autoIncrementKey2.fetch_add(step, memory_order_relaxed);
  }

  inline uint64_t ReadAutoIncrementKey2() {
    return _autoIncrementKey2.load(memory_order_relaxed);
  }

  inline void WriteAutoIncrementKey2(uint64_t key) {
    _autoIncrementKey2.store(key, memory_order_relaxed);
  }

  inline uint64_t GetAndAddAutoIncrementKey3(uint64_t step) {
    return _autoIncrementKey3.fetch_add(step, memory_order_relaxed);
  }

  inline uint64_t ReadAutoIncrementKey3() {
    return _autoIncrementKey3.load(memory_order_relaxed);
  }

  void WriteAutoIncrementKey3(uint64_t key) {
    _autoIncrementKey3.store(key, memory_order_relaxed);
  }

  inline uint16_t ReadKeyVariableFieldCount() {
    return _keyAlterableFieldCount;
  }

  void WriteKeyVariableFieldCount(uint16_t num);

  inline uint16_t ReadValueVariableFieldCount() {
    return _valueAlterableFieldCount;
  }

  void WriteValueVariableFieldCount(uint16_t num);

  void ReadGarbage(uint32_t &totalPage, PageID &pageId, uint16_t &usedPage) {
    pageId = ReadInt(GARBAGE_PAGE_OFFSET);
    usedPage = ReadShort(GARBAGE_SAVE_PAGES_NUM_OFFSET);
    totalPage = ReadInt(GRABAGE_TOTAL_PAGES_OFFSET);
  }

  void WriteGabage(uint32_t totalPage, PageID pageId, int16_t usedPage) {
    WriteInt(GARBAGE_PAGE_OFFSET, pageId);
    WriteShort(GARBAGE_SAVE_PAGES_NUM_OFFSET, usedPage);
    WriteInt(GRABAGE_TOTAL_PAGES_OFFSET, totalPage);
  }
};
} // namespace storage
