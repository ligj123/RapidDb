#pragma once
#include <atomic>
#include "../header.h"
#include <string>
#include <queue>
#include "../file/PageFile.h"
#include "HeadPage.h"
#include <atomic>
#include <unordered_set>
#include "../utils/SpinMutex.h"
#include "LeafRecord.h"
#include <unordered_map>
#include "RawKey.h"
#include "../utils/ErrorMsg.h"

namespace storage {
	using namespace std;
  class IndexTree
  {
	public:
		IndexTree(const string& tableName, const string& fileName, VectorDataValue& vctKey, VectorDataValue& vctVal);
		utils::ErrorMsg* InsertRecord(LeafRecord* rr);
		void UpdateRootPage(IndexPage* root);
		LeafRecord* GetRecord(const RawKey& key);
		void GetRecords(const RawKey& key, VectorLeafRecord& vct);
		void QueryRecord(RawKey* keyStart, RawKey* keyEnd,
			bool bIncLeft, bool bIncRight, VectorLeafRecord& vct);
		IndexPage* AllocateNewPage(uint64_t parentId, Byte pageLevel);
		void CloneKeys(VectorDataValue& vct);
		void CloneValues(VectorDataValue& vct);
		IndexPage* GetPage(uint64_t pageId, bool bLeafPage);
		PageFile* ApplyPageFile();

		inline uint64_t GetRecordsCount() { return _headPage->ReadTotalRecordCount(); }
		inline string& GetFileName() { return _fileName; }
		inline uint64_t GetFileId() { return _fileId; }
		inline bool IsClosed() { return _bClosed; }
		inline void SetClose() { _bClosed = true; }
		void Close(bool bWait);
		inline void ReleasePageFile(PageFile* rpf) {
			lock_guard<utils::SpinMutex> lock(_fileMutex);
			_fileQueue.push(rpf);
		}
		inline PageFile* GetOverflowFile() {
			if (_ovfFile == nullptr) {
				unique_lock< utils::SpinMutex> lock(_fileMutex);
				if (_ovfFile == nullptr) {
					string name = _fileName.substr(0, _fileName.find_last_of('.')) + "_ovf.dat";
					_ovfFile = new PageFile(name, true);
				}
			}
			return _ovfFile;
		}
		inline HeadPage* GetHeadPage() { return _headPage; }
		inline void IncPages() { ++_pageCountInPool; }
		inline void DecPages() {
				--_pageCountInPool;
			if (_pageCountInPool.load() == 0) delete this;
		}
		inline void IncTask() { _taskWaiting.fetch_add(1); }
		inline void DecTask() { _taskWaiting.fetch_sub(1); }
		inline int64_t GetTaskWaiting() { return _taskWaiting.load(); }
	protected:
		~IndexTree();
		LeafPage* SearchRecursively(const RawKey& key);
		LeafPage* SearchRecursively(const LeafRecord& lr);
	protected:
		static std::atomic<uint32_t> _atomicFileId;
		std::string _tableName;
		std::string _fileName;
		std::queue<PageFile*> _fileQueue;
		utils::SpinMutex _fileMutex;
		/**How much page files were opened for this index tree*/
		uint32_t _rpfCount = 0;
		uint64_t _fileId;
		PageFile* _ovfFile = nullptr;
		bool _bClosed = false;
		/** Head page */
		HeadPage* _headPage = nullptr;
		
		/** To record how much pages are in index page pool */
		atomic<int64_t> _pageCountInPool = 0;
		/** How much pages are waiting to read or write */
		atomic<int64_t> _taskWaiting = 0;
		
		VectorDataValue _vctKey;
		VectorDataValue _vctValue;
		utils::SpinMutex _pageMutex;
		unordered_map<uint64_t, utils::SpinMutex*> _mapMutex;
		queue<utils::SpinMutex*> _queueMutex;
		/**To lock for root page*/
		utils::SharedSpinMutex _rootSharedMutex;
		IndexPage* _rootPage = nullptr;
	protected:
		static unordered_set<uint16_t> _setFiledId;
		static uint16_t _currFiledId;
		static utils::SpinMutex _spinMutex;
  };
}
