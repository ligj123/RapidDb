#include "Configure.h"

namespace storage {
  const uint64_t Configure::DEFULT_DISK_CLUSTER_SIZE = 4 * 1024;
  const uint64_t Configure::DEFAULT_CACHE_PAGE_SIZE = Configure::DEFULT_DISK_CLUSTER_SIZE * 4;
  const uint64_t Configure::DEFAULT_TOTAL_CACHE_SIZE = 8 * 1024 * 1024 * 1024LL;
  const uint64_t Configure::DEFAULT_CACHE_BLOCK_SIZE = 128 * 1024;
  const uint64_t Configure::DEFAULT_MAX_BUFFER_SIZE = 1024 * 1024 * 1024;
  const uint64_t Configure::DEFAULT_MAX_RECORD_LENGTH = 8000;
  const uint64_t Configure::DEFAULT_MAX_KEY_LENGTH = 2000;
  const uint64_t Configure::DEFAULT_MAX_COLUMN_LENGTH = 1024 * 1024 * 1024;
  const uint64_t Configure::DEFAULT_MAX_FREE_BUFFER_COUNT = 1000;
  const uint64_t Configure::MAX_PAGE_FILE_COUNT = 5;
  const uint64_t Configure::MAX_OVERFLOW_CACHE_SIZE = 1024 * 1024;
  Configure* Configure::instance = []() {return new Configure; }();
  //const uint64_t WRITE_DELAY_MS = 10 * 1000;
  //const uint64_t MAX_QUEUE_SIZE = 10000;
  //const uint64_t DEFAULT_DISK_CACHE_PAGE_SIZE = 1024 * 1024;
  //const uint64_t MAX_DISK_CACHE_PAGE_COUNT = 2048;
  //const char* DEFAULT_DISK_CACHE_FILE = { "./" }; 

  Configure::Configure()
  {
    _szDiskCluster = DEFULT_DISK_CLUSTER_SIZE;
    _szCachePage = DEFAULT_CACHE_PAGE_SIZE;
    _szTotalCache = DEFAULT_TOTAL_CACHE_SIZE;
    _szCacheBlock = DEFAULT_CACHE_BLOCK_SIZE;
    _szMaxBuffer = DEFAULT_MAX_BUFFER_SIZE;
    _lenMaxRecord = DEFAULT_MAX_RECORD_LENGTH;
    _lenMaxKey = DEFAULT_MAX_KEY_LENGTH;
    _lenMaxColumn = DEFAULT_MAX_COLUMN_LENGTH;
    _countMaxFreeBuff = DEFAULT_MAX_FREE_BUFFER_COUNT;
    _countMaxPageFile = MAX_PAGE_FILE_COUNT;
    _maxOverflowCache = MAX_OVERFLOW_CACHE_SIZE;
  }
}