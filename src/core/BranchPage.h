#pragma once
#include "IndexPage.h"
#include "RawKey.h"

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
		~BranchPage();

		void CleanRecords();
		void ReadPage() override;
		void WritePage() override;
		void LoadRecords();
		BranchRecord* DeleteRecord(uint16_t index);
		bool SaveRecord() override;

		void InsertRecord(BranchRecord*& record, int32_t pos = -1);
		BranchRecord* DeleteRecord(const BranchRecord& record);
		bool RecordExist(const RawKey& key) const;

		bool AddRecord(BranchRecord*& record);
		bool IsPageFull() const { return _totalDataLength >= MAX_DATA_LENGTH;	}

		uint16_t GetMaxDataLength() const override {	return MAX_DATA_LENGTH;	}

		uint32_t SearchRecord(const BranchRecord& rr) const;
		uint32_t SearchKey(const RawKey& key) const;
		BranchRecord* GetRecordByPos(int32_t pos);
	protected:
		int CompareTo(uint32_t recPos, const BranchRecord& rr) const;
		int CompareTo(uint32_t recPos, const RawKey& key) const;
  };
}
