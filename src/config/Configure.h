#pragma once
#include <string>
#include <mutex>
namespace storage {
	class Configure
	{
	public:
		/**The default size of disk cluster */
		static const uint64_t DEFULT_DISK_CLUSTER_SIZE;
		/**The default size of a cache page for index tree*/
		static const uint64_t DEFAULT_CACHE_PAGE_SIZE;
		/**The default total allocated memory size for cache*/
		static const uint64_t DEFAULT_TOTAL_CACHE_SIZE;
		/**A block size for memory cache, it will used to create DataValueInt etc.*/
		static const uint64_t DEFAULT_CACHE_BLOCK_SIZE;
		/**The max memory size of blocks used to allocate IDataValues.*/
		static const uint64_t DEFAULT_MAX_MEM_DATAVALUE_SIZE;
		/**The max memory size of blocks used to */
		static const uint64_t DEFAULT_MAX_MEM_RECORD_SIZE;
		/**The max size for a record, key length + the value data length without fields saved to overflow file.
		* It can not exceed the half of cache page.*/
		static const uint64_t DEFAULT_MAX_RECORD_LENGTH;
		/**The max size of a record key. It can not excced the half of a record's size.*/
		static const uint64_t DEFAULT_MAX_KEY_LENGTH;
		/**The default max length for variable columns*/
		static const uint64_t DEFAULT_MAX_COLUMN_LENGTH;
		/**The free memory buffer in cache pool's queue*/
		static const uint64_t DEFAULT_MAX_FREE_BUFFER_COUNT;
		//static const uint64_t WRITE_DELAY_MS = 10 * 1000;
		//static const uint64_t MAX_QUEUE_SIZE = 10000;
		//static const uint64_t DEFAULT_DISK_CACHE_PAGE_SIZE = 1024 * 1024;
		//static const uint64_t MAX_DISK_CACHE_PAGE_COUNT = 2048;
		//static const char* DEFAULT_DISK_CACHE_FILE = { "./" }; //System.getProperty("java.io.tmpdir");
		//static const uint64_t MAX_RANDOM_FILE_NUM = 5;

	public:
		Configure();
		static uint64_t GetDiskClusterSize() { return GetInstance()._szDiskCluster; }
		static uint64_t GetCachePageSize() { return GetInstance()._szCachePage; }
		static uint64_t GetTotalCacheSize() { return GetInstance()._szTotalCache; }
		static uint64_t GetCacheBlockSize() { return GetInstance()._szCacheBlock; }
		static uint64_t GetMaxMemDataValueSize() { return GetInstance()._szMaxMemDataValue; }
		static uint64_t GetMaxMemRecordSize() { return GetInstance()._szMaxMemRecord; }
		static uint64_t GetMaxRecordLength() { return GetInstance()._lenMaxRecord; }
		static uint64_t GetMaxKeyLength() { return GetInstance()._lenMaxKey; }
		static uint64_t GetMaxColumnLength() { return GetInstance()._lenMaxColumn; }
		static uint64_t GetMaxFreeBufferCount() {	return GetInstance()._countMaxFreeBuff; }
	protected:
		static Configure& GetInstance() {
			return *instance;
		}
	protected:
		static Configure* instance;

		uint64_t _szDiskCluster;
		uint64_t _szCachePage;
		uint64_t _szTotalCache;
		uint64_t _szCacheBlock;
		uint64_t _szMaxMemDataValue;
		uint64_t _szMaxMemRecord;
		uint64_t _lenMaxRecord;
		uint64_t _lenMaxKey;
		uint64_t _lenMaxColumn;
		uint64_t _countMaxFreeBuff;
	};
}