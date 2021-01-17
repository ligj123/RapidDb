#pragma once
#include <unordered_map>
#include "../core/IndexPage.h"
#include "../utils/SpinMutex.h"

namespace storage {
	using namespace std;
  class IndexPagePool
  {
	protected:
		static unordered_map<uint64_t, IndexPage*> _mapCache;
		static utils::SharedSpinMutex _rwLock;
		static uint64_t _maxCacheSize;
		static thread _tIndexPageManager;
		static uint64_t _prevDelNum;

	protected:
		static void PoolManage();
	public:
		static uint64_t GetMaxCacheSize() {	return _maxCacheSize;	}
		static void SetMaxCacheSzie(uint64_t size) { _maxCacheSize = (int)size;	}

		static void AddPage(IndexPage* page);

		static IndexPage* GetPage(uint64_t fileId, uint64_t offset) {
			return GetPage(CachePage::CalcHashCode(fileId, offset));
		}

		static IndexPage* GetPage(long pageId);
		/**Only used for test to remove results from previous test cases*/
		static void CleanPool();

		static int getCacheSize() {	return _mapCache.size(); }


//		static int testCount = 0;
//		static {
//			Runnable r = new Runnable(){
//				public void run() {
//					try {
//						cacheManage();
//					}
//	 catch (Exception ex) {
//	log.error("Error for cache manage", ex);
//}
//}
//			};
//			cachePoolManager.scheduleAtFixedRate(r, 5, 5, TimeUnit.SECONDS);
//		}

  };
}