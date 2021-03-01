#include "IndexTree.h"
#include "../pool/StoragePool.h"
#include "../utils/Log.h"
#include "BranchRecord.h"
#include "BranchPage.h"
#include "../pool/PageBufferPool.h"
#include "IndexPage.h"
#include <shared_mutex>

namespace storage {
	unordered_set<uint16_t> IndexTree::_setFiledId;
	uint16_t IndexTree::_currFiledId = 0;
	utils::SpinMutex IndexTree::_spinMutex;

	IndexTree::IndexTree(const string& tableName, const string& fileName,
		VectorDataValue& vctKey, VectorDataValue& vctVal) {
		_tableName = tableName;
		_fileName = fileName;
		for (auto iter = _fileName.begin(); iter != _fileName.end(); iter++) {
			if (*iter == '\\') *iter = '/';
		}

		{
			lock_guard<utils::SpinMutex> lock(_spinMutex);
			while (true) {
				if (_setFiledId.find(_currFiledId) == _setFiledId.end()) {
					_setFiledId.insert(_currFiledId);
					_fileId = _currFiledId;
					break;
				}
				_currFiledId++;
			}
		}

		fstream fs(_fileName);
		bool bExist = fs.good();
		_headPage = new HeadPage(this);
		if (!bExist) {
			{
				unique_lock<utils::ReentrantSharedSpinMutex> lock(_headPage->GetLock());
				memset(_headPage->GetBysPage(), 0, Configure::GetDiskClusterSize());
				_headPage->WriteFileVersion();
				_headPage->WriteRootPagePointer(0);
				_headPage->WriteTotalPageCount(0);
				_headPage->WriteTotalRecordCount(0);
				_headPage->WriteAutoPrimaryKey(0);
			}

			StoragePool::WriteCachePage(_headPage);
			_rootPage = AllocateNewPage(HeadPage::NO_PARENT_POINTER, 0);
		}
		else {
			_headPage->ReadPage();
			FileVersion&& fv = _headPage->ReadFileVersion();
			if (!(fv == INDEX_FILE_VERSION)) {
				throw ErrorMsg(TB_ERROR_INDEX_VERSION, { _fileName });
			}

			uint64_t rootId = _headPage->ReadRootPagePointer();
			_rootPage = GetPage(rootId, rootId == 0);
		}

		_vctKey.swap(vctKey);
		_vctValue.swap(vctVal);
		LOG_DEBUG << "Open index tree " << tableName;
	}

	IndexTree::~IndexTree() {
		unique_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
		if (_headPage->IsDirty()) {
			_headPage->WritePage();
		}

		while (_queueMutex.size() > 0) {
			delete _queueMutex.front();
			_queueMutex.pop();
		}

		for (auto iter = _mapMutex.begin(); iter != _mapMutex.end(); iter++) {
			delete iter->second;
		}

		if (_ovfFile != nullptr) delete _ovfFile;
		while (_fileQueue.size() > 0) {
			delete _fileQueue.front();
			_fileQueue.pop();
		}

		delete _headPage;
	}

	void IndexTree::Close(bool bWait) {
		unique_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
		_bClosed = true;
		_rootPage->DecRefCount();
		if (!bWait) return;

		while (_taskWaiting.load() > 0) {
			this_thread::sleep_for(chrono::milliseconds(1));
		}

		if (_headPage->IsDirty()) {
			_headPage->WritePage();
		}

		while (_queueMutex.size() > 0) {
			delete _queueMutex.front();
			_queueMutex.pop();
		}
		
		for (auto iter = _mapMutex.begin(); iter != _mapMutex.end(); iter++) {
			delete iter->second;
		}
		_mapMutex.clear();

		if (_ovfFile != nullptr) delete _ovfFile;
		while (_fileQueue.size() > 0) {
			delete _fileQueue.front();
			_fileQueue.pop();
		}
	}

	void IndexTree::CloneKeys(VectorDataValue& vct)
	{
		vct.RemoveAll();
		vct.reserve(_vctKey.size());
		for (IDataValue* dv : _vctKey) {
			vct.push_back(dv->CloneDataValue(false));
		}
	}

	void IndexTree::CloneValues(VectorDataValue& vct)
	{
		vct.RemoveAll();
		vct.reserve(_vctValue.size());
		for (IDataValue* dv : _vctValue) {
			vct.push_back(dv->CloneDataValue(false));
		}
	}

	PageFile* IndexTree::ApplyPageFile() {
		while (true) {
			{
				lock_guard<utils::SpinMutex> lock(_fileMutex);
				if (_fileQueue.size() > 0) {
					PageFile* rpf = _fileQueue.front();
					_fileQueue.pop();
					return rpf;
				}
				else if (_rpfCount < Configure::GetMaxPageFileCount()) {
					_rpfCount++;
					return new PageFile(_fileName);
				}
			}

			this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void IndexTree::InsertRecord(LeafRecord* rr) {
		RawKey* key = rr->GetKey();
		LeafPage* page = SearchRecursively(true, *key);
		delete key;

		page->InsertRecord(rr);

		if (page->GetTotalDataLength() > LeafPage::MAX_DATA_LENGTH * 5U) {
			page->PageDivide();
		}
		page->WriteUnlock();
		page->DecRefCount();
	}

	IndexPage* IndexTree::GetPage(uint64_t pageId, bool bLeafPage) {
		assert(pageId < _headPage->ReadTotalPageCount());
		IndexPage* page = PageBufferPool::GetPage(_fileId, pageId);
		if (page != nullptr) return page;

		_pageMutex.lock();

		utils::SpinMutex* lockPage = nullptr;
		auto iter = _mapMutex.find(pageId);
		if (iter != _mapMutex.end()) {
			lockPage = iter->second;
		}
		else {
			lockPage = new utils::SpinMutex;
			_mapMutex.insert(pair<uint64_t, utils::SpinMutex* >(pageId, lockPage));
		}

		_pageMutex.unlock();
		lockPage->lock();

		page = PageBufferPool::GetPage(_fileId, pageId);
		if (page == nullptr) {
			if (bLeafPage) {
				page = new LeafPage(this, pageId);
			}
			else {
				page = new BranchPage(this, pageId);
			}

			std::future<int> fut = StoragePool::ReadCachePage(page);
			fut.get();

			page->Init();
			PageBufferPool::AddPage(page);
			page->IncRefCount();
			IncPages();
		}

		_pageMutex.lock();
		_queueMutex.push(lockPage);
		_mapMutex.erase(pageId);
		lockPage->unlock();
		_pageMutex.unlock();

		return page;
	}

	void IndexTree::UpdateRootPage(IndexPage* root) {
		unique_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
		_rootPage->DecRefCount();
		_headPage->WriteRootPagePointer(root->GetPageId());
		_rootPage = root;
		root->IncRefCount();
		StoragePool::WriteCachePage(_headPage);
	}

	LeafRecord* IndexTree::GetRecord(const RawKey& key) {
		LeafPage* page = SearchRecursively(false, key);
		LeafRecord* rr = page->GetRecord(key);

		page->ReadUnlock();
		page->DecRefCount();
		return rr;
	}

	void IndexTree::GetRecords(const RawKey& key, VectorLeafRecord& vct ) {
		vct.reserve(256);
		LeafPage* page = SearchRecursively(false, key);
		while (true) {
			VectorLeafRecord vctLr;
			page->GetRecords(key, vctLr);
			vct.insert(vct.end(), vctLr.begin(), vctLr.end());
			
			LeafRecord* lastLr = page->GetLastRecord();
			if (lastLr != nullptr && lastLr->CompareKey(key) > 0) {
				lastLr->ReleaseRecord();
				break;
			}

			if (lastLr != nullptr) {
				lastLr->ReleaseRecord();
			}

			uint64_t nextId = page->GetNextPageId();
			if (nextId == HeadPage::NO_NEXT_PAGE_POINTER)
				break;

			LeafPage* nextPage = (LeafPage*)GetPage(nextId, true);
			nextPage->ReadLock();
			page->ReadUnlock();
			page->DecRefCount();
			page = nextPage;
		}

		page->ReadUnlock();
		page->DecRefCount();
	}

	void IndexTree::QueryRecord(RawKey* keyStart, RawKey* keyEnd,
		bool bIncLeft, bool bIncRight, VectorLeafRecord& vctRt) {
		vctRt.reserve(256);
		LeafPage* page = nullptr;
		uint32_t pos = 0;
		if (keyStart == nullptr) {
			uint64_t id = _headPage->ReadBeginLeafPagePointer();
			page = (LeafPage*)GetPage(id, true);
			page->ReadLock();
		}
		else {
			page = SearchRecursively(false, *keyStart);
			bool bFind;
			pos = page->SearchKey(*keyStart, bFind);
		}

		bool bStart = true;
		while (true) {
			VectorLeafRecord vct;
			page->GetAllRecords(vct);
			for (uint32_t i = 0; i < pos; i++) {
				vct[i]->ReleaseRecord();
			}

			if (keyStart != nullptr && bStart && !bIncLeft) {
				for (; pos < vct.size(); pos++) {
					if (vct[pos]->CompareKey(*keyStart) != 0)
						break;

					vct[pos]->ReleaseRecord();
				}

				if (pos >= vct.size()) {
					uint64_t idNext = page->GetNextPageId();
					if (idNext == HeadPage::NO_NEXT_PAGE_POINTER) {
						break;
					}
					LeafPage* pageNext = (LeafPage*)GetPage(idNext, true);
					pageNext->ReadLock();
					page->ReadUnlock();
					page->DecRefCount();
					page = pageNext;
					pos = 0;
					continue;
				}
				else {
					bStart = false;
				}
			}

			int count = 0;
			for (; pos < vct.size(); pos++) {
				LeafRecord* rr = vct[pos];
				if (keyEnd != nullptr) {
					int hr = rr->CompareKey(*keyEnd);
					if ((!bIncRight && hr >= 0) || hr > 0) {
						break;
					}
				}

				count++;
				vctRt.push_back(rr);
			}

			if (pos < vct.size()) {
				for (; pos < vct.size(); pos++) {
					vct[pos]->ReleaseRecord();
				}

				break;
			}

			uint64_t nextId = page->GetNextPageId();
			if (nextId == HeadPage::NO_PARENT_POINTER)
				break;

			LeafPage* nextPage = (LeafPage*)GetPage(nextId, true);
			nextPage->ReadLock();
			page->ReadUnlock();
			page->DecRefCount();
			page = nextPage;
			pos = 0;
		}

		page->ReadUnlock();
		page->DecRefCount();
	}

	IndexPage* IndexTree::AllocateNewPage(uint64_t parentId, Byte pageLevel) {
		uint64_t newPageId = _headPage->GetAndIncTotalPageCount();
		IndexPage* page = nullptr;
		if (0 != pageLevel) {
			page = new BranchPage(this, newPageId, pageLevel, parentId);
		}
		else {
			page = new LeafPage(this, newPageId, parentId);
		}

		PageBufferPool::AddPage(page);
		page->IncRefCount();
		IncPages();

		LOG_DEBUG << "Allocate new CachePage, pageLevel=" << (int)pageLevel << "  pageId=" << newPageId;
		return page;
	}

	LeafPage* IndexTree::SearchRecursively(bool isEdit, const RawKey& key) {
		IndexPage* page = nullptr;
		while (true) {
			{
				std::shared_lock<utils::SharedSpinMutex> lock(_rootSharedMutex);
				bool b = false;
				if (isEdit && _rootPage->GetPageLevel() == 0) {
					b = _rootPage->WriteTryLock();
				}
				else {
					b = _rootPage->ReadTryLock();
				}

				if (b) {
					page = _rootPage;
					page->IncRefCount();
					break;
				}
			}

			this_thread::sleep_for(std::chrono::microseconds(1));
		}

		bool bNonunique = (_headPage->ReadIndexType() == IndexType::NON_UNIQUE);
		while (true) {
			if (page->GetPageLevel() == 0) {
				return (LeafPage*)page;
			}

			uint64_t pageId = UINT64_MAX;
			if (bNonunique) {
				BranchPage* bPage = (BranchPage*)page;
				bool bFind;
				uint32_t pos = bPage->SearchKey(key, bFind);
				BranchRecord* br = bPage->GetRecordByPos(pos);
				if (isEdit) {
					if (br->CompareKey(key) == 0 || pos == 0) {
						pageId = br->GetChildPageId();
					}
					else {
						BranchRecord* br2 = bPage->GetRecordByPos(pos + 1);
						if (br2 != nullptr && br->CompareKey(*br2) == 0) {
							pageId = bPage->GetRecordByPos(pos - 1)->GetChildPageId();
						}
						else {
							pageId = br->GetChildPageId();
						}
						if (br2 != nullptr)	br2->ReleaseRecord();
					}
				}
				else {
					BranchRecord* br2 = bPage->GetRecordByPos(pos + 1);
					if (br2 != nullptr && br->CompareKey(*br2) == 0) {
						pageId = bPage->GetRecordByPos(pos - 1)->GetChildPageId();
					}
					else {
						pageId = br->GetChildPageId();
					}
				}

				br->ReleaseRecord();
			}
			else {
				BranchPage* bPage = (BranchPage*)page;
				bool bFind;
				uint32_t pos = bPage->SearchKey(key, bFind);
				BranchRecord* br = bPage->GetRecordByPos(pos);
				pageId = ((BranchRecord*)br)->GetChildPageId();
				br->ReleaseRecord();
			}

			IndexPage* childPage = (IndexPage*)GetPage(pageId, page->GetPageLevel() == 1);
			if (childPage == nullptr) {
				page->ReadUnlock();
				return nullptr;
			}

			if (isEdit && typeid(childPage) == typeid(LeafPage)) {
				childPage->WriteLock();
			}
			else {
				childPage->ReadLock();
			}

			page->ReadUnlock();
			page->DecRefCount();
			page = childPage;
		}
	}
}