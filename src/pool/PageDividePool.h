#pragma once
#include <unordered_map>
#include "../core/IndexPage.h"
#include <thread>
#include "../utils/SpinMutex.h"
#include "../core/IndexTree.h"
#include "../utils/ThreadPool.h"

namespace storage {
	using namespace std;

	class PageDividePool
	{
	public:
		static const uint64_t MAX_QUEUE_SIZE;
		static const uint32_t BUFFER_FLUSH_INTEVAL_MS;
		static const int SLEEP_INTEVAL_MS;
		static const float PAGE_DIVIDE_LIMIT;

		static unordered_map<uint64_t, IndexPage*> _mapPage;
		static queue<IndexPage*> _queuePage;
		static thread _treeDivideThread;
		static bool _bSuspend;
		static utils::SpinMutex _spinMutex;

		static thread CreateThread();
		static void AddCachePage(IndexPage* page);
		static void SetThreadStatus(bool bSuspend) { _bSuspend = bSuspend; }

		static size_t FetQueuePageSize() {return _queuePage.size();	}
	};
}