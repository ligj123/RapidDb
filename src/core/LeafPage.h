#pragma once
#include "IndexPage.h"
#include "RawKey.h"

namespace storage {
	class LeafRecord;
	class VectorLeafRecord;

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
		~LeafPage();

		inline void SetPrevPageId(uint64_t id) {_prevPageId = id;	}
		inline uint64_t GetPrevPageId() {	return _prevPageId;	}
		inline void SetNextPageId(uint64_t id) {	_nextPageId = id;	}
		inline uint64_t GetNextPageId() { return _nextPageId; }
		inline bool IsPageFull() { return _totalDataLength >= MAX_DATA_LENGTH; }
		
		void LoadRecords();
		void CleanRecord();
		bool SaveRecord() override;
		/**Insert a leaf record to this page, if pos < 0, use SearchRecord to find the position,
		if pos>=0, insert the position or add to edn*/
		void InsertRecord(LeafRecord* lr, int32_t pos = -1);
		bool AddRecord(LeafRecord* record);
		void DeleteRecord(int32_t pos);
		void UpdateRecord(int32_t pos);
		void GetAllRecords(VectorLeafRecord& vct);
		LeafRecord* GetLastRecord();
		LeafRecord* GetRecord(const RawKey& key);
		void GetRecords(const RawKey& key, VectorLeafRecord& vct);
		int32_t SearchRecord(const LeafRecord& rr, bool& bFind);
		int32_t SearchKey(const RawKey& key, bool& bFind);
		uint16_t GetMaxDataLength() const override { return MAX_DATA_LENGTH; }
	protected:
		inline LeafRecord* GetVctRecord(int pos) const {
			return (LeafRecord*)_vctRecord[pos];
		}
		int CompareTo(uint32_t recPos, const RawKey& key);
		int CompareTo(uint32_t recPos, const LeafRecord& rr);
	protected:
		uint64_t _prevPageId;
		uint64_t _nextPageId;
	};
}