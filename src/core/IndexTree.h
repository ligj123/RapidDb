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

namespace storage {
	using namespace std;
  class IndexTree
  {
	public:
		IndexTree(const string& tableName, const string& fileName, VectorDataValue& vctKey, VectorDataValue& vctVal);
		~IndexTree();
		void Close();
		void InsertRecord(LeafRecord* rr);
		void UpdateRootPage(IndexPage* root);
		LeafRecord* GetRecord(const RawKey& key);
		void GetRecords(const RawKey& key, VectorLeafRecord& vct);
		void QueryRecord(RawKey* keyStart, RawKey* keyEnd,
			bool bIncLeft, bool bIncRight, VectorLeafRecord& vct);
		IndexPage* AllocateNewPage(uint64_t parentId, Byte pageLevel);

		uint64_t GetRecordsCount() { return _headPage->ReadTotalRecordCount(); }
		string& GetFileName() { return _fileName; }
		uint64_t GetFileId() { return _fileId; }
		bool IsClosed() { return _bClosed; }
		void setClosed() { _bClosed = true;	}
		PageFile* ApplyPageFile();
		void ReleasePageFile(PageFile* rpf) {
			lock_guard<utils::SpinMutex> lock(_fileMutex);
			_fileQueue.push(rpf);
		}
		PageFile* GetOverflowFile() {
			if (_ovfFile == nullptr) {
				unique_lock< utils::SpinMutex> lock(_pageMutex);
				if (_ovfFile == nullptr) {
					string name = _fileName.substr(0, _fileName.find_last_of('.')) + "_ovf.dat";
					_ovfFile = new PageFile(name, true);
				}
			}
			return _ovfFile;
		}
		HeadPage* GetHeadPage() { return _headPage; }
		uint32_t IncreaseTasks() { return _tasksWaiting.fetch_add(1); }
		uint32_t DecreaseTasks() { return _tasksWaiting.fetch_sub(1);	}
		void CloneKeys(VectorDataValue& vct);
		void CloneValues(VectorDataValue& vct);
		IndexPage* GetPage(uint64_t pageId, bool bLeafPage);
	protected:
		LeafPage* SearchRecursively(bool isEdit, const RawKey& key);
	protected:
		static std::atomic<uint32_t> _atomicFileId;
		std::string _tableName;
		std::string _fileName;
		std::queue<PageFile*> _fileQueue;
		utils::SpinMutex _fileMutex;
		/**How much page files have been opened for this index tree*/
		uint32_t _rpfCount;
		uint64_t _fileId;
		PageFile* _ovfFile;
		bool _bClosed;
		/** Head page */
		HeadPage* _headPage;
		
		/** To record how much pages are waiting to read or write */
		std::atomic<uint32_t> _tasksWaiting;
		
		VectorDataValue _vctKey;
		VectorDataValue _vctValue;
		utils::SpinMutex _pageMutex;
		unordered_map<uint64_t, utils::SpinMutex*> _mapMutex;
		queue<utils::SpinMutex*> _queueMutex;
		/**To lock for root page*/
		utils::SharedSpinMutex _rootSharedMutex;
		IndexPage* _rootPage;
	protected:
		static unordered_set<uint16_t> _setFiledId;
		static uint16_t _currFiledId;
		static utils::SpinMutex _spinMutex;
  };
}
