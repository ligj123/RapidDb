#pragma once
#include "RawRecord.h"
#include "LeafPage.h"
#include "../dataType/IDataValue.h"

namespace storage {
	class LeafPage;
  class LeafRecord : public RawRecord
  {
	protected:
		~LeafRecord();
	
	public:
		LeafRecord(LeafPage* indexPage, Byte* bys);
		LeafRecord(IndexTree* indexTree, Byte* bys);
		LeafRecord(const LeafRecord& src) = delete;
		LeafRecord(IndexTree* indexTree, const VectorDataValue& vctKey, const VectorDataValue& vctVal,
			uint64_t offset = UINT64_MAX, uint32_t oldSizeOverflow = UINT32_MAX);
		void CleanUndoRecord();
		void AddOldRecord(LeafRecord* old);
		void ReleaseRecord() { _refCount--; if (_refCount == 0) delete this; }
		LeafRecord* ReferenceRecord() { _refCount++; return this; }
		ActionType GetActionType() { return _actionType;	}
		void SetActionType(ActionType type) { _actionType = type; }
		bool IsTranFinished() { return _bTranFinished; }
		void SetTranFinished(bool bFinished) { _bTranFinished = bFinished; }

		uint64_t GetTrasactionId() { return _tranId; }
		void SetTransaction(uint64_t id) { _tranId = id; }
		
		uint16_t GetValueLength() const {
			return (*((uint16_t*)_bysVal) - *((uint16_t*)(_bysVal + sizeof(uint16_t))) - sizeof(uint16_t) * 3);
		}

		uint16_t GetIndexOvfStart() const {	return *((uint16_t*)(_bysVal + sizeof(uint16_t) * 2)); }

		uint64_t GetOvfOffset() const {
			if (*((uint16_t*)(_bysVal + sizeof(uint16_t))) != UINT16_MAX)
				return *((uint64_t*)(_bysVal + GetTotalLength() - 12));
			else
				return 0;
		}

		uint32_t GetSizeOverflow() {
			if (*((uint16_t*)(_bysVal + sizeof(uint16_t))) != UINT16_MAX)
				return *((uint32_t*)(_bysVal + GetTotalLength() - 4));
			else
				return -1;
		}

		void GetListKey(VectorDataValue& vct) const ;
		void GetListValue(VectorDataValue& vct) const;
		void GetListOverflow(vector<IDataValue*>& vctVal) const;

		int CompareTo(const LeafRecord& lr) const;
		int CompareKey(const RawKey& key) const;
		int CompareKey(const LeafRecord& lr) const;
		RawKey* GetKey() const;

		uint16_t SaveData(Byte* bysPage) {
			uint16_t len = GetTotalLength();
			std::memcpy(bysPage, _bysVal, len);
			return len;
		}

	protected:
		/**Save old records for recover*/
		LeafRecord* _undoRecords;
		/**The transaction id if it is running in a transaction, or UINT64_MAX*/
		uint64_t _tranId;

		friend std::ostream& operator<< (std::ostream& os, const LeafRecord& lr);
	};

	std::ostream& operator<< (std::ostream& os, const LeafRecord& lr);
	class VectorLeafRecord : public vector<LeafRecord*> {
	public:
		using vector::vector;
		~VectorLeafRecord() {
			for (auto iter = begin(); iter != end(); iter++) {
				(*iter)->ReleaseRecord();
			}
		}

		void clear() {
			for (auto iter = begin(); iter != end(); iter++) {
				(*iter)->ReleaseRecord();
			}
		}
	};
}

