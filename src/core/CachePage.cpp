#include "CachePage.h"
#include "IndexTree.h"
#include "../cache/CachePool.h"

namespace storage {
  const uint32_t CachePage::CRC32_INDEX_OFFSET = Configure::GetCachePageSize() - sizeof(uint64_t);
  const uint32_t CachePage::CRC32_HEAD_OFFSET = Configure::GetDiskClusterSize() - sizeof(uint64_t);

	CachePage::CachePage(IndexTree* indexTree, uint64_t pageId) {
		_indexTree = indexTree;
		_pageId = pageId;
		_fileId = _indexTree->GetFileId();
		if (pageId == UINT64_MAX) {
			_bysPage = new Byte[Configure::GetDiskClusterSize()];
		}
		else {
			_bysPage = CachePool::ApplyPage();
		}
	}

	bool CachePage::IsFileClosed() {
		return _indexTree->IsFileClosed();
	}

	void CachePage::ReadPage() {
		std::lock_guard<utils::SharedSpinMutex> lock(_rwLock);
		
		PageFile* pageFile = _indexTree->ApplyPageFile();
		if (_pageId != UINT64_MAX) {
			pageFile->ReadPage(Configure::GetDiskClusterSize() + _pageId * Configure::GetCachePageSize(),
				(char*)_bysPage, Configure::GetCachePageSize());
		} else {
			pageFile->ReadPage(0, (char*)_bysPage, Configure::GetDiskClusterSize());
		}
		_indexTree->ReleasePageFile(pageFile);
		_bDirty = false;
	}

	void CachePage::WritePage() {
		PageFile* pageFile = _indexTree->ApplyPageFile();
		std::lock_guard<utils::SharedSpinMutex> lock(_rwLock);

		if (_pageId != UINT64_MAX) {
			pageFile->WritePage(Configure::GetDiskClusterSize() + _pageId * Configure::GetCachePageSize(),
				(char*)_bysPage, Configure::GetCachePageSize());
		}	else {
			pageFile->WritePage(0, (char*)_bysPage, Configure::GetDiskClusterSize());
		}
		_indexTree->ReleasePageFile(pageFile);
		_bDirty = false;
	}
}