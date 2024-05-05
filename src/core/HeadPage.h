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
  /**The max stamp versions can be support at the same time in current version*/
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
  // When create a table, its root page, begin leaf page and end leaf page
  // shouble be 0.
  /**The root page id for this index*/
  PageID _rootPageId{0};
  /**The begin leaf page for this index*/
  PageID _beginLeafPageId{0};
  /**The end leaf page for this index*/
  PageID _endLeafPageId{0};
  /**The count of total records in this index*/
  uint64_t _totalRecordCount{0};
  /**To generate the auto increment ids, the value is current id*/
  uint64_t _autoIncrementKey{0};
  // In this table, any changes for a record will need a new record version
  // stamp, it start from 0, and will add one every time. This stamp will be
  // used for log transport, data snapshot etc.
  uint64_t _currRecordStamp{0};

public:
  HeadPage(IndexTree *indexTree, uint32_t fileId)
      : CachePage(indexTree, UINT32_MAX, PageType::HEAD_PAGE, fileId) {
    _bysPage = CachePool::Apply(HEAD_PAGE_SIZE);
  }
  ~HeadPage() { CachePool::Release(_bysPage, HEAD_PAGE_SIZE); }
  // Create a new head page and initialize it.
  void InitHeadPage(IndexType iType, const VectorDataValue &vctVal);
  // After read head page and load parameters from the buffer.
  void InitParameters() override;
  bool SaveToBuffer() override;
  uint32_t PageSize() const override { return HEAD_PAGE_SIZE; }

  void WriteFileVersion();
  FileVersion ReadFileVersion();

  /**
   * @brief Apply one or more record stamps. For single task only one stamp to
   * apply every time and does not need atomic. For multi tasks, should apply a
   * batch of stamps every time to save time and need use atomic method.
   * @param num The number of stamps to apply
   * @return The start of record stamps.
   */
  inline VersionStamp GetAndIncRecordStamp(uint64_t num = 1) {
    _bDirty = true;
    if (num == 1) {
      return _currRecordStamp++;
    } else {
      return atomic_ref<VersionStamp>(_currRecordStamp)
          .fetch_add(num, memory_order_relaxed);
    }
  }
  /**
   * @brief Here does not consider data consistency, the caller should consider
   * it.
   */
  inline VersionStamp ReadRecordStamp() { return _currRecordStamp; }
  // Set current record stamp when recover table from crash
  void SetRecordStamp(VersionStamp recordStamp) {
    _currRecordStamp = recordStamp;
    _bDirty = true;
  }
  /**
   * @brief Change the total page count and return start page id.
   * @param pageNum The number to change
   * @param atm If use atomic method. For single task, only one thread to use
   * this head page, do not need atomic method. For multi tasks, the related
   * sttements split into multi groups and execute in multi task in one index
   * tree.
   * @return The start page id
   */
  inline PageID GetAndIncTotalPageCount(uint32_t pageNum = 1,
                                        bool atm = false) {
    _bDirty = true;
    if (!atm) {
      PageID id = _totalPageCount;
      _totalPageCount += pageNum;
      return id;
    } else {
      return atomic_ref<uint32_t>(_totalPageCount)
          .fetch_add(pageNum, memory_order_relaxed);
    }
  }

  /**
   * @brief Here does not consider data consistency, the caller should consider
   * it.
   */
  inline uint32_t ReadTotalPageCount() { return _totalPageCount; }
  /**
   * @brief Only used to recover table from crash
   */
  void SetTotalPageCount(uint32_t totalPage) {
    _totalPageCount = totalPage;
    _bDirty = true;
  }
  /**
   * @brief Add the new record number and return the total numbers after added.
   * @param recNum The record number to update.
   * @param atm
   */
  inline uint64_t IncAndGetTotalRecordCount(uint64_t recNum = 1,
                                            bool atm = false) {
    _bDirty = true;
    if (atm) {
      uint64_t val = atomic_ref<uint64_t>(_totalRecordCount)
                         .fetch_add(recNum, memory_order_relaxed);
      return val + recNum;
    } else {
      _totalRecordCount += recNum;
      return _totalRecordCount;
    }
  }
  /**
   * @brief Read total record count. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline uint64_t ReadTotalRecordCount() { return _totalRecordCount; }
  /**
   * @brief Only used to recover table from crash
   */
  inline void SetTotalRecordCount(uint64_t totalRecNum) {
    _totalRecordCount = totalRecNum;
    _bDirty = true;
  }
  /**
   * @brief Set root page. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline void SetRootPageID(PageID pid) {
    _rootPageId = pid;
    _bDirty = true;
  }
  /**
   * @brief Get root page. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline PageID GetRootPageID() { return _rootPageId; }
  /**
   * @brief Set begin leaf page. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline void SetBeginLeafPageID(PageID pid) {
    _beginLeafPageId = pid;
    _bDirty = true;
  }
  /**
   * @brief Get begin leaf page. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline PageID GetBeginLeafPageID() { return _beginLeafPageId; }
  /**
   * @brief Set end leaf page. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline void SetEndLeafPageID(PageID pid) {
    _endLeafPageId = pid;
    _bDirty = true;
  }
  /**
   * @brief Get begin leaf page. Here does not consider data consistency,
   * the caller should consider it.
   */
  inline uint64_t GetEndLeafPagePointer() { return _endLeafPageId; }
  /**
   * @brief Get Index Type.
   */
  inline IndexType GetIndexType() { return _indexType; }

  /**
   * @brief Add auto increment key and return start key.
   * @param pageNum The number to keys
   * @param atm If use atomic method. For single task, only one thread to use
   * this head page, do not need atomic method. For multi tasks, the related
   * sttements split into multi groups and execute in multi task in one index
   * tree.
   * @return The start key
   */
  inline uint64_t GetAndAddAutoIncrementKey(uint64_t step = 1) {
    _bDirty = true;
    if (step == 1) {
      _autoIncrementKey++;
      return _autoIncrementKey;
    } else {
      return atomic_ref<uint64_t>(_autoIncrementKey)
          .fetch_add(step, memory_order_relaxed);
    }
  }
  /**
   * @brief Get current auto increment key. Here does not consider data
   * consistency, the caller should consider it.
   */
  inline uint64_t GetAutoIncrementKey() { return _autoIncrementKey; }

  inline void SetAutoIncrementKey(uint64_t key) {
    _autoIncrementKey = key;
    _bDirty = true;
  }

  inline uint16_t GetKeyVariableFieldCount() { return _keyAlterableFieldCount; }

  inline uint16_t GetValueVariableFieldCount() {
    return _valueAlterableFieldCount;
  }
  /**
   * @brief Read garbage information from buffer. The caller should make sure
   * data consistency.
   */
  void ReadGarbage(uint32_t &totalPage, PageID &pageId, uint16_t &usedPage,
                   uint32_t &crc32) {
    pageId = ReadInt(GARBAGE_PAGE_OFFSET);
    usedPage = ReadShort(GARBAGE_PAGES_NUM_OFFSET);
    totalPage = ReadInt(GRABAGE_TOTAL_ITEMS_OFFSET);
    crc32 = ReadInt(GARBAGE_CRC32_OFFSET);
  }
  /**
   * @brief Write garbage information from buffer. The caller should make sure
   * data consistency.
   */
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
