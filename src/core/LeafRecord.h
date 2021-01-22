#pragma once
#include "RawRecord.h"
#include "LeafPage.h"
#include <vector>
#include "ActionType.h"

namespace storage {
	class LeafPage;
  class LeafRecord : public RawRecord
  {
	protected:
		/** the byte array that save key and value's content */
		Byte* _bysVal;
		/** the parent page included this record */
		LeafPage* _parentPage;
		/**index tree*/
		IndexTree* _indexTree;
		/**Save old records for recover*/
		LeafRecord* _undoRecords;
		/**The transaction id if it is running in a transaction, or UINT64_MAX*/
		uint64_t _tranId;
		uint32_t _refCount;
		/***/
		ActionType _actionType;
		/**In current transaction if this record has been executed.*/
		bool _bTranFinished;
		/**If this record' value is saved to solely buffer or branch page*/
		bool _bSole;
	protected:
		~LeafRecord();
	
	public:
		LeafRecord(LeafPage* indexPage, Byte* bys);
		LeafRecord(IndexTree* indexTree, Byte* bys);
		LeafRecord(const LeafRecord& src);
		LeafRecord(IndexTree* indexTree, const vector<IDataValue*>& vctKey, const vector<IDataValue*>& vctVal);
		LeafRecord(IndexTree* indexTree, const vector<IDataValue*>& vctKey, const vector<IDataValue*>& vctVal,
			uint64_t offset, uint32_t oldSizeOverflow);
		void CleanUndoRecord();
		void ReleaseRecord() { _refCount--; if (_refCount == 0) delete this; }
		LeafRecord* ReferenceRecord() { _refCount++; return this; }
		void AddOldRecord(LeafRecord* old);
		ActionType GetActionType() { return _actionType;	}
		void SetActionType(ActionType type) { _actionType = type; }
		bool IsTranFinished() { return _bTranFinished; }
		void SetTranFinished(bool bFinished) { _bTranFinished = bFinished; }

		uint64_t GetTrasactionId() { return _tranId; }
		void SetTransaction(uint64_t id) { _tranId = id; }
		Byte* GetBysValue() const override { return _bysVal; }
		uint16_t GetTotalLength() const override { return *((uint16_t*)_bysVal);}

		uint16_t GetKeyLength() const override { return *((uint16_t*)(_bysVal + sizeof(uint16_t) * 2));	}

		uint16_t GetValueLength() const override {
			return *((uint16_t*)_bysVal) - *((uint16_t*)(_bysVal + sizeof(uint16_t) * 2));
		}

		IndexPage* GetParentPage() const override { return _parentPage;	}
		void SetParentPage(IndexPage* page) override { _parentPage = (LeafPage*)page;	}
		IndexTree* GetTreeeFile() const override { return _indexTree;	}

		uint16_t GetIndexOvfStart() const {	return *((uint16_t*)(_bysVal + sizeof(uint16_t))); }

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

		vector<IDataValue*>& GetListKey() const override;
		vector<IDataValue*>& GetListValue() const override;
		void GetListOverflow(vector<IDataValue*>& vctVal) const;

		int CompareTo(const RawRecord& other) const override;
		int CompareKey(const RawKey& key) const override;
		int CompareKey(const RawRecord& other) const override;
		RawKey* GetKey() const;

		void SaveData(Byte* bysPage) override { std::memcpy(bysPage, _bysVal, GetTotalLength()); }
		friend std::ostream& operator<< (std::ostream& os, const LeafRecord& lr);
	};

	std::ostream& operator<< (std::ostream& os, const LeafRecord& lr);
}

