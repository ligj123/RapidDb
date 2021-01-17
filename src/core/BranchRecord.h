#pragma once
#include "RawRecord.h"
#include "BranchPage.h"
#include "IndexType.h"

namespace storage {
  class BranchRecord : public RawRecord
  {
	public:
		/**Page Id length*/
		static const uint32_t PAGE_ID_LEN;
	protected:
		/** the byte array that save key and value's content */
		Byte* _bysVal;
		/** the parent page included this record */
		BranchPage* _parentPage;
		/**tree file*/
		IndexTree* _indexTree;
		/**If this record' value is saved to solely buffer or branch page*/
		bool _bSole;

	public:
		BranchRecord(BranchPage* parentPage, Byte* bys) : _parentPage(parentPage), _bysVal(bys),
										_indexTree(parentPage->GetIndexTree()), _bSole(false) {	}

		BranchRecord(IndexTree* indexTree, RawRecord* rec, long childPageId);
		BranchRecord(const BranchRecord& src);
		~BranchRecord();
		RawKey* GetKey() const override;
		vector<IDataValue*>& GetListKey() const override;
		vector<IDataValue*>& GetListValue() const override;
		int CompareTo(const RawRecord& other) const override;
		int CompareKey(const RawKey& key) const override;
		int CompareKey(const RawRecord& other) const override;
		bool EqualPageId(const BranchRecord& br) const;

		inline Byte* GetBysValue() const  override { return _bysVal; }
		inline uint16_t GetTotalLength() const override { return *((uint16_t*)_bysVal); }
		inline uint16_t GetKeyLength() const override {	return *((uint16_t*)(_bysVal + sizeof(uint16_t)));	}
		inline uint16_t GetValueLength() const override {
			return *((uint16_t*)_bysVal) - sizeof(uint16_t) * 2 - PAGE_ID_LEN	- *((uint16_t*)(_bysVal + sizeof(uint16_t)));
		}
		inline IndexPage* GetParentPage() const override {	return _parentPage;	}
		inline void SetParentPage(IndexPage* page) override { _parentPage = (BranchPage*)page; }
		inline uint64_t GetChildPageId() const { return *((uint64_t*)(_bysVal + GetTotalLength() - PAGE_ID_LEN)); }
		inline IndexTree* GetTreeeFile() const override {	return _indexTree; }
		inline void SaveData(Byte* bysPage) override { std::memcpy(bysPage, _bysVal, GetTotalLength()); }

		friend std::ostream& operator<< (std::ostream& os, const BranchRecord& br);		
  };

	std::ostream& operator<< (std::ostream& os, const BranchRecord& br);
}

