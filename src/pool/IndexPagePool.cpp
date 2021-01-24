#include "IndexPagePool.h"
#include <shared_mutex>
#include "../core/IndexTree.h"
#include "../utils/Log.h"

namespace storage {
	unordered_map<uint64_t, IndexPage*> IndexPagePool::_mapCache;
	utils::SharedSpinMutex IndexPagePool::_rwLock;
	uint64_t IndexPagePool::_maxCacheSize = 10000;
	thread IndexPagePool::_tIndexPageManager;
	uint64_t IndexPagePool::_prevDelNum = 100;

	void IndexPagePool::AddPage(IndexPage* page) {
		unique_lock<utils::SharedSpinMutex> lock(_rwLock);
		_mapCache.insert(pair<uint64_t, IndexPage*>(page->HashCode(), page));
	}

	IndexPage* IndexPagePool::GetPage(uint64_t pageId) {
		shared_lock<utils::SharedSpinMutex> lock(_rwLock);
		auto iter = _mapCache.find(pageId);
		if (iter != _mapCache.end()) {
			iter->second->UpdateAccessTime();
			iter->second->IncRefCount();
			return iter->second;
		}
		
		return nullptr;
	}

	void IndexPagePool::CleanPool() {
		unique_lock<utils::SharedSpinMutex> lock(_rwLock);
		_mapCache.clear();
	}

	void IndexPagePool::PoolManage() {
		uint64_t numDel = _mapCache.size() - _maxCacheSize;
			if (numDel <= 0) {
				return;
			}

			if (numDel > _prevDelNum * 2) {
				numDel = _prevDelNum * 2;
				if (numDel > 100000)
					numDel = 100000;
			}
			else if (numDel < _prevDelNum / 2)
			{
				numDel = _prevDelNum / 2;
				if (numDel < 500)
					numDel = 500;
			}

			_prevDelNum = numDel;
			_rwLock.lock_shared();
			auto iter = _mapCache.begin();

			auto cmp = [](IndexPage* left, IndexPage* right) { return left->GetAccessTime() < right->GetAccessTime(); };
			priority_queue<IndexPage*, vector<IndexPage*>, decltype(cmp)> queue(cmp);

			int refCountPage = 0;
			int refPageCount = 0;

			while (iter != _mapCache.end()) {
				IndexPage* page = iter->second;
				int num = page->GetRefCount();
				if (num > 0) {
					refCountPage++;
					refPageCount += num;
				}
				else if (num < 0) {
					LOG_ERROR << "Errr page ref count. Id=" << page->getPageId() << "\trefCount=" << num;
				}

				if (!page->Releaseable()) {
					continue;
				}

				if (queue.size() < numDel) {
					queue.push(page);
				}
				else if (page->GetAccessTime() < queue.top()->GetAccessTime()) {
					queue.push(page);
					queue.pop();
				}
			}

			int delCount = 0;
			_rwLock.unlock_shared();
			_rwLock.lock();
			while (queue.size() > 0) {
				IndexPage* page = queue.top();
				queue.pop();

				if (!page->Releaseable()) {
					continue;
				}

				_mapCache.erase(page->HashCode());
				delete page;
				delCount++;
			}
			_rwLock.unlock();
		}
}