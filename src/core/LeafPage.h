#pragma once
#include "IndexPage.h"

namespace storage {
	class LeafRecord;
  class LeafPage : public IndexPage
  {
	public:
		static uint16_t PREV_PAGE_POINTER_OFFSET;
		static uint16_t NEXT_PAGE_POINTER_OFFSET;
		static uint16_t DATA_BEGIN_OFFSET;
		static uint16_t MAX_DATA_LENGTH;

	public:
		LeafPage(IndexTree* indexTree, uint64_t pageId, uint64_t parentPageId);
		LeafPage(IndexTree* indexTree, uint64_t pageId);

		void SetPrevPageId(uint64_t id) {_prevPageId = id;	}
		uint64_t GetPrevPageId() {	return _prevPageId;	}
		void SetNextPageId(uint64_t id) {	_nextPageId = id;	}
		uint64_t GetNextPageId() { return _nextPageId; }
		
		vector<LeafRecord*>* GetAllRecords();
		LeafRecord* GetLastRecord();
		void LoadRecords();
		void CleanRecord();
		bool SaveRecord();
		void InsertRecord(LeafRecord* lr);
		bool AddRecord(LeafRecord* record);
		LeafRecord* GetRecord(const RawKey& key);
		vector<LeafRecord*>* GetRecords(const RawKey& key);
		uint32_t SearchRecord(const LeafRecord& rr, bool bEqual);
		uint32_t SearchKey(const RawKey& key, bool bEqual);
		bool IsPageFull() { return _totalDataLength >= MAX_DATA_LENGTH; }
		uint16_t GetMaxDataLength() { return MAX_DATA_LENGTH; }
	protected:
		int CompareTo(uint32_t recPos, const RawKey& key);
		int CompareTo(uint32_t recPos, const LeafRecord& rr);
	protected:
		uint64_t _prevPageId;
		uint64_t _nextPageId;
	};
}