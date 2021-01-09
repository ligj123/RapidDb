#include "IndexPage.h"
#include "BranchPage.h"

namespace storage {
	 float IndexPage::LOAD_FACTOR = 0.8f;
	 short IndexPage::PAGE_LEVEL_OFFSET = 0;
	 short IndexPage::NUM_RECORD_OFFSET = 2;
	 short IndexPage::TOTAL_DATA_LENGTH_OFFSET = 4;
	 short IndexPage::PARENT_PAGE_POINTER_OFFSET = 8;

	 IndexPage::IndexPage(IndexTree* indexTree, uint64_t pageId) : CachePage(indexTree, pageId) {
	 }

	 IndexPage::IndexPage(IndexTree* indexTree, uint64_t pageId, uint32_t pageLevel, uint64_t parentPageId)
		 : CachePage(indexTree, pageId) {
		 _vctRecord.clear();
		 _recordRefCount.store(0, std::memory_order_relaxed);
		 _recordNum = 0;
		 _totalDataLength = 0;
		 _bysPage[PAGE_LEVEL_OFFSET] = (Byte)pageLevel;
		 _dtPageLastUpdate = GetMsSinceEpoch();
		 _parentPageId = parentPageId;
	 }

	 void IndexPage::LoadPage() {
		 _vctRecord.clear();
		 _recordRefCount.store(0, std::memory_order_relaxed);
		 _recordNum = ReadShort(NUM_RECORD_OFFSET);
		 _totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
		 _parentPageId = ReadLong(PARENT_PAGE_POINTER_OFFSET);
		 _dtPageLastUpdate = GetMsSinceEpoch();
	 }
}