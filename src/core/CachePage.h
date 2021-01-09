#pragma once
#include "../config/Configure.h"
#include "../config/ErrorID.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include <atomic>
#include <chrono>
#include "../utils/BytesConvert.h"
#include "../cache/CachePool.h"

namespace storage {
	class IndexTree;

  class CachePage
  {
	public:
		static const uint32_t CRC32_INDEX_OFFSET;
		static const uint32_t CRC32_HEAD_OFFSET;
		inline static uint64_t CalcHashCode(uint64_t fileId, uint64_t pageId) {
			return (fileId << 32) + pageId;
		}
		inline static uint64_t GetMsSinceEpoch() {
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		}

	public:
		static void* operator new(size_t size)
		{
			return CachePool::Apply((uint32_t)size);
		}
		static void operator delete(void* ptr, size_t size)
		{
			CachePool::Release((Byte*)ptr, (uint32_t)size);
		}

	public:
		CachePage(IndexTree* indexTree, uint64_t pageId);
		bool IsFileClosed();

		inline utils::SharedSpinMutex& GetLock() { return _rwLock; }
		inline bool IsDirty() { return _bDirty; }
		inline void SetDirty(bool b) { _bDirty = b; }
		inline bool IsLocked() { return _rwLock.is_locked(); }
		inline uint64_t getPageId() {	return _pageId; }
		inline void UpdateAccessTime() { _dtPageLastAccess = GetMsSinceEpoch();	}
		inline uint64_t HashCode() { return CalcHashCode(_fileId, _pageId); }
		inline void UpdateWriteTime() { _dtPageLastWrite = GetMsSinceEpoch(); }
		inline uint64_t GetWriteTime() { return _dtPageLastWrite; }
		inline uint64_t GetFileId() { return _fileId;	}
		inline uint64_t IncRefCount() { return _refCount.fetch_add(1); }
		inline uint64_t DecRefCount() { return _refCount.fetch_sub(1); }
		inline uint64_t GetRefCount() { return _refCount; }
		inline IndexTree* getIndexTree() { return _indexTree; }
		inline Byte* GetBysPage() { return _bysPage; }
		virtual void Release() = 0;
		virtual bool releaseable() { return _refCount == 0; }
		virtual void ReadPage();
		virtual void WritePage() ;

		inline Byte ReadByte(uint32_t pos) {
			assert(Configure::GetCachePageSize() > sizeof(Byte) + pos);
			return _bysPage[pos];
		}

		inline void WriteByte(uint32_t pos, Byte value) {
			assert(Configure::GetCachePageSize() > sizeof(Byte) + pos);
			_bysPage[pos] = value;
		}

		inline int16_t ReadShort(uint32_t pos) {
			assert(Configure::GetCachePageSize() > sizeof(int16_t) + pos);
			return utils::Int16FromBytes(_bysPage + pos);
		}

		inline void WriteShort(uint32_t pos, int16_t value) {
			assert(Configure::GetCachePageSize() > sizeof(int16_t) + pos);
			utils::Int16ToBytes(value, _bysPage + pos);
		}

		inline int32_t ReadInt(uint32_t pos) {
			assert(Configure::GetCachePageSize() > sizeof(int32_t) + pos);
			return utils::Int32FromBytes(_bysPage + pos);
		}

		inline void WriteInt(uint32_t pos, int32_t value) {
			assert(Configure::GetCachePageSize() > sizeof(int32_t) + pos);
			utils::Int32ToBytes(value, _bysPage + pos);
		}

		uint64_t ReadLong(uint32_t pos) {
			assert(Configure::GetCachePageSize() > sizeof(int64_t) + pos);
			return utils::Int64FromBytes(_bysPage + pos);
		}

		void WriteLong(uint32_t pos, int64_t value) {
			assert(Configure::GetCachePageSize() > sizeof(int64_t) + pos);
			utils::Int64ToBytes(value, _bysPage + pos);
		}

	protected:
		Byte* _bysPage;
		bool _bDirty;
		utils::SharedSpinMutex _rwLock;
		uint64_t _dtPageLastWrite;
		uint64_t _dtPageLastAccess;
		uint64_t _pageId;
		uint64_t _fileId;
		IndexTree* _indexTree;
		std::atomic<uint64_t> _refCount;
  };
}

