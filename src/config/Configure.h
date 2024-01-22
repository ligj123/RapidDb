#pragma once
#include <mutex>
#include <string>

using namespace std;
namespace storage {
/**According disk type, here will use different type to write to or read data
 * from disk. SSD is more efficient with multi threads and HDD is more efficient
 * with sequence read or write. So here will use different ways to read or write
 * page blocks. For SSD, will use synchronous way to read. For HDD, will create
 * a task for read and all tasks will be added into a queue and sort them with
 * page id, then read page blocks ordered by page ids and will pause write task.
 */
enum class DiskType : int8_t { UNKNOWN = 0, HDD, SSD };

// Memory status
enum class MemoryStatus : int8_t {
  AMPLE = 0, // The used memory is less than 70%, do not release index page.
  INTENSE,   // The used memory is less than 90 and more than 70%, the unused
             // memory should be released.
  CRITICAL,  // The used memory has exceed 90%, LeafPage and other block should
             // be released soon after used.
  FATAL // The used memory has exceed the assigned value, can not assign new
        // memory.
};

class Configure {
public:
  static bool LoadConfig(const string &cfg);

  // The following value defined in program, can not be update in configure.
  //  The bytes of every disk cluster.
  static uint64_t GetDiskClusterSize() { return 4 * 1024; }
  // The page size in index, include primary and secondary index
  static uint64_t GetIndexPageSize() { return 16 * 1024; }
  // The page size to save result set
  static uint64_t GetResultPageSize() { return 1024 * 1024; }
  // The block size in cache pool
  static uint64_t GetCacheBlockSize() { return 128 * 1024; }
  // The max length of a record, include key and value saved in Index Page, if
  // exceed this length, the value will be saved into overflowpage.
  static uint64_t GetMaxRecordLength() { return 8180; }
  // The max key length, the all column's length sum + 2 bytes to save key
  // length
  static uint64_t GetMaxKeyLength() { return 2000; }
  // The max length of a column,
  static uint64_t GetMaxColumnLength() { return 1024 * 1024 * 1024; }
  // The max free memory block in CachePool. If exceed this number, the exceed
  // block will be freed.
  static uint64_t GetMaxFreeBufferCount() { return 128; }
  // The max free blocks for result set. If exceed this number, the blocks will
  // be free.
  static uint64_t GetMaxFreeResultBlock() { return 64; }

  // Following global variable can be update by configure file.
  // The total memory size can be used by this software.
  static uint64_t GetTotalMemorySize() { return GetInstance()._szTotalMemory; }
  // Auto transaction timeout(microseconds)
  static uint64_t GetAutoTaskTimeout() {
    return GetInstance()._autoTranTimeout;
  }
  // Manual transaction timeout(microseconds)
  static uint64_t GetManualTaskTimeout() {
    return GetInstance()._manualTranTimeout;
  }
  // If save the original page content when write log for split index page
  static bool IsLogSaveSplitPage() { return GetInstance()._bLogSaveSplitPage; }
  // Only used for distributed system. Will assign a unique id for a node. In
  // single system, it will always be 0.
  static uint16_t GetNodeId() { return GetInstance()._nodeId; }
  // The path to write log.
  static const string &GetLogPath() { return GetInstance()._strLogPath; }
  // The max log file size.
  static uint32_t GetBinLogFileSize() { return GetInstance()._binLogFileSize; }
  // Disk type
  static DiskType GetDiskType() { return GetInstance()._diskType; }
  // The database root path, all db data will be saved into here
  static const string &GetDbRootPath() { return GetInstance()._strDbRootPath; }
  static MemoryStatus GetMemoryStatus() { return GetInstance()._memStatus; }
  static void SetMemoryStatus(MemoryStatus s) { GetInstance()._memStatus = s; }

protected:
  static Configure &GetInstance() {
    if (instance == nullptr) {
      instance = new Configure;
    }
    return *instance;
  }

protected:
  static Configure *instance;

  uint64_t _szTotalMemory{8 * 1024 * 1024 * 1024LL};
  // Auto transaction timeout(microseconds)
  uint64_t _autoTranTimeout{100 * 1000000};
  // Manual transaction timeout(microseconds)
  uint64_t _manualTranTimeout{1800 * 1000000};

  bool _bLogSaveSplitPage{false};
  DiskType _diskType{DiskType::SSD};
  // The max log file size.
  uint32_t _binLogFileSize{10 * 1024 * 1024};
  // For distribute, every node will assign a unique id to indentify the nodes.
  // In single environment, the node id=0
  uint16_t _nodeId;
  string _strLogPath;
  // The database root path, all db data will be saved into here
  string _strDbRootPath;
  // Memory status
  MemoryStatus _memStatus{MemoryStatus::AMPLE};
};
} // namespace storage
