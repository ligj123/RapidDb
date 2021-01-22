#pragma once
#include "CachePage.h"
#include "RawRecord.h"
#include "IndexType.h"

namespace storage {
	class BranchPage;
	class IndexPage : public CachePage
	{
	public:
		static const float LOAD_FACTOR;
		static const uint16_t PAGE_LEVEL_OFFSET;
		static const uint16_t PAGE_REFERENCE_COUNT;
		static const uint16_t NUM_RECORD_OFFSET;
		static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
		static const uint16_t PARENT_PAGE_POINTER_OFFSET;

	protected:
		vector<RawRecord*> _vctRecord;
		uint32_t _totalDataLength;
		uint32_t _recordNum = 0;
		uint64_t _parentPageId;
		uint64_t _dtPageLastUpdate;

	public:
		IndexPage(IndexTree* indexTree, uint64_t pageId);
		IndexPage(IndexTree* indexTree, uint64_t pageId, uint8_t pageLevel, uint64_t parentPageId);
	
		void LoadPage();
		bool PageDivide();

		void ClearRecords();
		inline void SetParentPageID(uint64_t parentPageId) { _parentPageId = parentPageId; }
		inline uint64_t GetParentPageId() { return _parentPageId; }
		inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
		inline uint64_t GetPageLastUpdateTime() { return _dtPageLastUpdate; }
		inline void SetPageLastUpdateTime() { _dtPageLastUpdate = GetMsFromEpoch(); }
		inline uint32_t GetTotalDataLength() { return _totalDataLength; }
		inline uint32_t GetRecordSize() { return _recordNum; }

		/**
		 * Save changed records information from mapRecord to byte array
		 * @return saved this page to byte array or not
		 */
		virtual bool SaveRecord() = 0;

		bool Releaseable() override { return _refCount.load() == 0; }

		virtual uint16_t GetMaxDataLength() = 0;
	};
}