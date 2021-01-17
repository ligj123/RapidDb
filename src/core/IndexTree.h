#pragma once
#include <atomic>
#include "../header.h"
#include <string>
#include <queue>
#include "../file/PageFile.h"
#include "HeadPag.h"
#include <atomic>

namespace storage {
  class IndexTree
  {
	public:
		uint64_t GetFileId() { return _fileId; }
		bool IsFileClosed() { return bClosed; }
		PageFile* ApplyPageFile() { return nullptr; }
		void ReleasePageFile(PageFile* rpf) {	}
		PageFile* GetOverflowFile() { return nullptr; }
		HeadPage* GetHeadPage() { return _headPage; }
		uint32_t IncreaseTasks() { return _tasksWaiting.fetch_add(1); }
		uint32_t DecreaseTasks() { return _tasksWaiting.fetch_sub(1);	}
		vector<IDataValue*>& CloneKeys();
		vector<IDataValue*>& CloneValues();
	protected:
		static std::atomic<uint32_t> _atomicFileId;
		std::string _filePathName;
		std::queue<PageFile*> _fileQueue;
		std::atomic<uint32_t> _rpfCount;
		uint64_t _fileId;
		PageFile _ovfFile;
		bool bClosed;
		/** Head page */
		HeadPage* _headPage;
		
		/** To record how much pages are waiting to read or write */
		std::atomic<uint32_t> _tasksWaiting;
		
		vector<IDataValue*> _vctKey;
		vector<IDataValue*> _vctValue;
  };
}
