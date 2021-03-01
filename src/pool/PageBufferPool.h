#pragma once
#include <unordered_map>
#include "../core/IndexPage.h"
#include "../utils/SpinMutex.h"

namespace storage {
	using namespace std;
  class PageBufferPool
  {
	protected:
		static unordered_map<uint64_t, IndexPage*> _mapCache;
		static utils::SharedSpinMutex _rwLock;
		static uint64_t _maxCacheSize;
		static thread _tIndexPageManager;
		static uint64_t _prevDelNum;
		static thread* _pageBufferThread;
		static bool _bSuspend;
	protected:
		static void PoolManage();
		static thread* CreateThread();
	public:
		static uint64_t GetMaxCacheSize() {	return _maxCacheSize;	}
		static void SetMaxCacheSzie(uint64_t size) { _maxCacheSize = (int)size;	}

		static void AddPage(IndexPage* page);

		static IndexPage* GetPage(uint64_t fileId, uint64_t offset) {
			return GetPage(CachePage::CalcHashCode(fileId, offset));
		}

		static IndexPage* GetPage(uint64_t pageId);
		/**Only used for test to remove results from previous test cases*/
		static void ClearPool();

		static uint64_t GetCacheSize() {	return _mapCache.size(); }
		static void SetSuspend(bool b) { _bSuspend = true; }
  };
}