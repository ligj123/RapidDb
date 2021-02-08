#pragma once
#include "CachePage.h"
#include "RawRecord.h"
#include "IndexType.h"

namespace storage {
	class IndexPage : public CachePage
	{
	public:
		static const float LOAD_FACTOR;
		static const uint16_t PAGE_LEVEL_OFFSET;
		static const uint16_t PAGE_REFERENCE_COUNT;
		static const uint16_t NUM_RECORD_OFFSET;
		static const uint16_t TOTAL_DATA_LENGTH_OFFSET;
		static const uint16_t PARENT_PAGE_POINTER_OFFSET;

	public:
		IndexPage(IndexTree* indexTree, uint64_t pageId);
		IndexPage(IndexTree* indexTree, uint64_t pageId, uint8_t pageLevel, uint64_t parentPageId);
		~IndexPage();

		bool PageDivide();

		inline void SetParentPageID(uint64_t parentPageId) { _parentPageId = parentPageId; }
		inline uint64_t GetParentPageId() { return _parentPageId; }
		inline Byte GetPageLevel() { return _bysPage[PAGE_LEVEL_OFFSET]; }
		inline uint64_t GetPageLastUpdateTime() { return _dtPageLastUpdate; }
		inline void SetPageLastUpdateTime() { _dtPageLastUpdate = GetMsFromEpoch(); }
		inline uint32_t GetTotalDataLength() { return _totalDataLength; }
		inline uint32_t GetRecordSize() { return _recordNum; }
		inline bool Releaseable() { return _refCount == 0 && _recordCount == 0; }
		inline int32_t IncRecordCount() { return ++_recordCount; }
		inline int32_t DecRecordCount() { return --_recordCount; }
		inline int32_t GetRecordCount() { return _recordCount; }

		virtual uint16_t GetMaxDataLength() const = 0;
		virtual bool SaveRecord() = 0;
	protected:
		vector<RawRecord*> _vctRecord;
		uint64_t _parentPageId = 0;
		uint64_t _dtPageLastUpdate = 0;
		int32_t _totalDataLength = 0;
		int32_t _recordNum = 0;
		int32_t _recordCount = 0;
	};
}