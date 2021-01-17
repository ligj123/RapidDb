#pragma once
#include "IndexPage.h"

namespace storage {
	class BranchRecord;

  class BranchPage : public IndexPage
  {
	public:
		static const uint16_t DATA_BEGIN_OFFSET;
		static const uint16_t MAX_DATA_LENGTH;

	public:
		BranchPage(IndexTree* indexTree, uint64_t pageId, Byte pageLevel, uint64_t parentId) :
			IndexPage(indexTree, pageId, pageLevel, parentId){ }

		BranchPage(IndexTree* indexTree, uint64_t pageId) :
			IndexPage(indexTree, pageId) {}

		void ReadPage() override;
		void WritePage() override;
		void LoadRecords();
		BranchRecord* DeleteRecord(uint16_t index);
		bool SaveRecord() override;

		bool InsertRecord(RawRecord* record);

		BranchRecord DeleteRecord(RawRecord* record);

		bool AddRecord(RawRecord* record) override;
		bool IsPageFull() override { return _totalDataLength >= MAX_DATA_LENGTH;	}

		uint16_t GetMaxDataLength() override {	return MAX_DATA_LENGTH;	}

		uint32_t SearchRecord(const BranchRecord& rr);
		uint32_t SearchKey(const RawKey& key);
	protected:
		int CompareTo(uint32_t recPos, const BranchRecord& rr);
		int CompareTo(uint32_t recPos, const RawKey& key);

		

  };
}
