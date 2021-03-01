#pragma once
#include "../utils/Log.h"
#include "../config/Configure.h"
#include "../utils/ThreadPool.h"
#include "../core/CachePage.h"
#include <queue>
#include <map>

namespace storage {
	using namespace std;

	class StoragePool
	{
	public:
		static void WriteCachePage(CachePage* page);
		static future<int> ReadCachePage(CachePage* page);
		static void FlushWriteCachePage() { _bWriteFlush = true; }
		static void SetWriteSuspend(bool b) { _bWriteSuspend = b; }
		static uint32_t GetWaitingWriteTaskCount() { return (uint32_t)_queueWrite.size(); }
	protected:
		static thread* CreateWriteThread();
	protected:
		static const uint32_t WRITE_DELAY_MS;
		static const uint64_t MAX_QUEUE_SIZE;
		static utils::ThreadPool _threadReadPool;

		static queue<CachePage*> _queueWrite;
		static map<uint64_t, CachePage*> _mapWrite;
		static thread* _threadWrite;
		static bool _bWriteFlush;
		static bool _bReadFirst;
		static utils::SpinMutex _spinMutex;
		static bool _bWriteSuspend;
	};
}