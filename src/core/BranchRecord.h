#pragma once
#include "RawRecord.h"
#include "../dataType/IDataValue.h"
#include "RawKey.h"
#include <cstring>

namespace storage {
	class BranchPage;
  class BranchRecord : public RawRecord
  {
	public:
		/**Page Id length*/
		static const uint32_t PAGE_ID_LEN;

	public:
		BranchRecord(BranchPage* parentPage, Byte* bys);
		BranchRecord(IndexTree* indexTree, RawRecord* rec, uint64_t childPageId);
		BranchRecord(const BranchRecord& src) = delete;
		~BranchRecord() {	}

		RawKey* GetKey() const;
		void GetListKey(VectorDataValue& vct) const;
		void GetListValue(VectorDataValue& vct) const;
		int CompareTo(const RawRecord& other) const;
		int CompareKey(const RawKey& key) const;
		int CompareKey(const RawRecord& other) const;
		bool EqualPageId(const BranchRecord& br) const;

		inline void ReleaseRecord() { _refCount--; if (_refCount == 0) delete this; }
		inline BranchRecord* ReferenceRecord() { _refCount++; return this; }
		uint16_t GetValueLength() const override {
			return (uint16_t)(*((uint16_t*)_bysVal) - sizeof(uint16_t) * 2 - PAGE_ID_LEN
				- *((uint16_t*)(_bysVal + sizeof(uint16_t))));
		}

		uint64_t GetChildPageId() const { return *((uint64_t*)(_bysVal + GetTotalLength() - PAGE_ID_LEN)); }
		uint16_t SaveData(Byte* bysPage) {
			uint16_t len = GetTotalLength();
			std::memcpy(bysPage, _bysVal, len);
			return  len;
		}

		friend std::ostream& operator<< (std::ostream& os, const BranchRecord& br);
	};

	std::ostream& operator<< (std::ostream& os, const BranchRecord& br);

	class VectorBranchRecord : public vector<BranchRecord*> {
	public:
		using vector::vector;
		~VectorBranchRecord() {
			for (auto iter = begin(); iter != end(); iter++) {
				(*iter)->ReleaseRecord();
			}
		}

		void RemoveAll() {
			for (auto iter = begin(); iter != end(); iter++) {
				(*iter)->ReleaseRecord();
			}

			clear();
		}
	};
}
