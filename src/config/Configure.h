#pragma once
#include <mutex>

namespace storage {
/**According disk type, here will use different type to write to or read data
 * from disk. SSD is more efficient with multi threads and HDD is more efficient
 * with sequence read or write. So here will use different ways to read or write
 * page blocks. For SSD, will use synchronous way to read. For HDD, will create
 * a task for read and all tasks will be added into a queue and sort them with
 * page id, then read page blocks ordered by page ids and will pause write task.
 */
enum class DiskType { UNKNOWN = 0, HDD, SSD };
class Configure {
public:
  /**The default size of disk cluster */
  static const uint64_t DEFULT_DISK_CLUSTER_SIZE;
  /**The default size of a cache page for index tree*/
  static const uint64_t DEFAULT_CACHE_PAGE_SIZE;
  /**The default total allocated memory size for cache*/
  static const uint64_t DEFAULT_TOTAL_CACHE_SIZE;
  /**A block size for memory cache, it will used to create DataValueInt etc.*/
  static const uint64_t DEFAULT_CACHE_BLOCK_SIZE;
  /**The max memory size of blocks used to allocate buffer, For example
   * IDataValue or Record.*/
  static const uint64_t DEFAULT_MAX_BUFFER_SIZE;
  /**The max size for a record, key length + the value data length without
   * fields saved to overflow file. It can not exceed the half of cache page.*/
  static const uint64_t DEFAULT_MAX_RECORD_LENGTH;
  /**The max size of a record key. It can not excced the half of a record's
   * size.*/
  static const uint64_t DEFAULT_MAX_KEY_LENGTH;
  /**The default max length for variable columns*/
  static const uint64_t DEFAULT_MAX_COLUMN_LENGTH;
  /**The free memory buffer in cache pool's queue*/
  static const uint64_t DEFAULT_MAX_FREE_BUFFER_COUNT;
  /**The max instances to open a index tree for reading or writing*/
  static const uint64_t MAX_PAGE_FILE_COUNT;
  /**The max size for overflow file cache*/
  static const uint64_t MAX_OVERFLOW_CACHE_SIZE;
  // static const uint64_t WRITE_DELAY_MS = 10 * 1000;
  // static const uint64_t MAX_QUEUE_SIZE = 10000;
  // static const uint64_t DEFAULT_DISK_CACHE_PAGE_SIZE = 1024 * 1024;
  // static const uint64_t MAX_DISK_CACHE_PAGE_COUNT = 2048;
  // static const char* DEFAULT_DISK_CACHE_FILE = { "./" };

  // The default wait time for automate transaction type, unit is millisecond.
  static const uint64_t AUTOMATE_TASK_OVERTIME;
  // The default wait time for manual transaction type, unit is millisecond.
  static const uint64_t MANUAL_TASK_OVERTIME;
  // If save the splited page's data into log file for database recovery.
  static const bool LOG_SAVE_SPLIT_PAGE;
  // Disk Type
  static const DiskType DISK_TYPE;

public:
  Configure();
  static uint64_t GetDiskClusterSize() { return GetInstance()._szDiskCluster; }
  static uint64_t GetCachePageSize() { return GetInstance()._szCachePage; }
  static uint64_t GetTotalCacheSize() { return GetInstance()._szTotalCache; }
  static uint64_t GetCacheBlockSize() { return GetInstance()._szCacheBlock; }
  static uint64_t GetMaxMemDataValueSize() {
    return GetInstance()._szMaxBuffer;
  }
  static uint64_t GetMaxRecordLength() { return GetInstance()._lenMaxRecord; }
  static uint64_t GetMaxKeyLength() { return GetInstance()._lenMaxKey; }
  static uint64_t GetMaxColumnLength() { return GetInstance()._lenMaxColumn; }
  static uint64_t GetMaxFreeBufferCount() {
    return GetInstance()._countMaxFreeBuff;
  }
  static uint64_t GetMaxPageFileCount() {
    return GetInstance()._countMaxPageFile;
  }
  static uint64_t GetMaxOverflowCache() {
    return GetInstance()._maxOverflowCache;
  }
  static uint64_t GetAutoTaskOvertime() {
    return GetInstance()._autoTaskOvertime;
  }
  static uint64_t GetManualTaskOvertime() {
    return GetInstance()._manualTaskOvertime;
  }

protected:
  static Configure &GetInstance() {
    if (instance == nullptr) {
      instance = new Configure;
    }
    return *instance;
  }

protected:
  static Configure *instance;

  uint64_t _szDiskCluster;
  uint64_t _szCachePage;
  uint64_t _szTotalCache;
  uint64_t _szCacheBlock;
  uint64_t _szMaxBuffer;
  uint64_t _lenMaxRecord;
  uint64_t _lenMaxKey;
  uint64_t _lenMaxColumn;
  uint64_t _countMaxFreeBuff;
  uint64_t _countMaxPageFile;
  uint64_t _maxOverflowCache;

  uint64_t _autoTaskOvertime;
  uint64_t _manualTaskOvertime;

  bool _bLogSaveSplitPage;
  DiskType _diskType;
};
} // namespace storage
