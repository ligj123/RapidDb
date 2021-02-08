#include "StoragePool.h"
#include "../core/CachePage.h"
#include "../core/IndexTree.h"

namespace storage {
  const uint32_t StoragePool::WRITE_DELAY_MS = 1 * 1000;
  const uint64_t StoragePool::MAX_QUEUE_SIZE = Configure::GetTotalCacheSize() / Configure::GetCachePageSize();

  utils::ThreadPool StoragePool::_threadReadPool("StoragePool", (uint32_t)MAX_QUEUE_SIZE, 1);

  queue<CachePage*> StoragePool::_queueWrite;
  map<uint64_t, CachePage*> StoragePool::_mapWrite;
	thread* StoragePool::_threadWrite = StoragePool::CreateWriteThread();
  bool StoragePool::_bWriteFlush = false;
  bool StoragePool::_bReadFirst = true;
	utils::SpinMutex StoragePool::_spinMutex;

	thread* StoragePool::CreateWriteThread() {
		thread* t = new thread([]() {
			utils::ThreadPool::_threadName = "WriteThread";
			while (true) {
				try {
					if ((_bReadFirst && _threadReadPool.GetTaskCount() > 0) || _queueWrite.size() == 0) {
						if (_queueWrite.size() == 0) _bWriteFlush = false;

						this_thread::sleep_for(std::chrono::milliseconds(1));
						continue;
					}

					if (!_bWriteFlush) {
						if (_queueWrite.size() < MAX_QUEUE_SIZE / 4) {
							this_thread::sleep_for(std::chrono::milliseconds(1));
						}
					}

					_spinMutex.lock();
					CachePage* page = _queueWrite.front();
					_queueWrite.pop();
					if (!page->IsDirty()) {
						_mapWrite.erase(page->HashCode());
						page->GetIndexTree()->DecreaseTasks();
						page->DecRefCount();
						_spinMutex.unlock();
						continue;
					}

					if (!_bWriteFlush && (_queueWrite.size() < MAX_QUEUE_SIZE / 2) && !page->IsFileClosed()
						&& (CachePage::GetMsFromEpoch() - page->GetWriteTime() < WRITE_DELAY_MS)) {
						_queueWrite.push(page);
						_spinMutex.unlock();
						continue;
					}

					_mapWrite.erase(page->HashCode());
					_spinMutex.unlock();
					page->WritePage();
					page->GetIndexTree()->DecreaseTasks();
					page->DecRefCount();
				}
				catch (...)
				{
					LOG_ERROR << "unknown exception!";
				}
			}
			});
		return t;
	}

	void StoragePool::WriteCachePage(CachePage* page) {
		{
			lock_guard< utils::SpinMutex> lock(_spinMutex);
			if (_mapWrite.find(page->HashCode()) != _mapWrite.end())
				return;

			page->GetIndexTree()->IncreaseTasks();
			page->UpdateWriteTime();
			_mapWrite.insert(pair<uint64_t, CachePage*>(page->HashCode(), page));
			_queueWrite.push(page);
			page->IncRefCount();
		}

		while (_mapWrite.size() > MAX_QUEUE_SIZE * 2) {
			this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	future<int> StoragePool::ReadCachePage(CachePage* page) {
			page->GetIndexTree()->IncreaseTasks();
			page->IncRefCount();
			future<int> fut = _threadReadPool.AddTask([page]() {
					page->ReadPage();
					page->GetIndexTree()->DecreaseTasks();
					page->DecRefCount();

					return 1;				
			});

			return fut;
		}

}