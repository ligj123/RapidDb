#include "LeafRecord.h"
#include "IndexTree.h"
#include "LeafPage.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"

namespace storage {
	LeafRecord::LeafRecord(LeafPage* parentPage, Byte* bys) {
		_parentPage = _parentPage;
		_bysVal = bys;
		_indexTree = _parentPage->GetIndexTree();
		_actionType = ActionType::UNKNOWN;
	}

	LeafRecord::LeafRecord(IndexTree* indexTree, Byte* bys) {
		_parentPage = nullptr;
		_bysVal = bys;
		_indexTree = indexTree;
		_actionType = ActionType::UNKNOWN;
	}


	LeafRecord::LeafRecord(const LeafRecord& src) {
		_parentPage = src._parentPage;
		_bysVal = src._bysVal;
		_indexTree = src._indexTree;
		_actionType = src._actionType;
		_tranId = src._tranId;
	}

	LeafRecord::LeafRecord(IndexTree* indexTree, const vector<IDataValue*>& vctKey, const vector<IDataValue*>& vctVal) {
		LeafRecord(indexTree, vctKey, vctVal, UINT64_MAX, UINT32_MAX);
	}

	LeafRecord::LeafRecord(IndexTree* indexTree, const vector<IDataValue*>& vctKey, const vector<IDataValue*>& vctVal,
		uint64_t offset, uint32_t oldSizeOverflow) {
		
		_parentPage = nullptr;
		_indexTree = indexTree;
		_actionType = ActionType::UNKNOWN;

		bool bPri = (indexTree->GetHeadPage()->ReadIndexType() == IndexType::PRIMARY);
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t valVarNum = bPri ? 0 : _indexTree->GetHeadPage()->ReadValueVariableFieldCount();

		int i;
		uint16_t lenKey = keyVarNum * sizeof(uint16_t);
		for (i = 0; i < vctKey.size(); i++) {
			lenKey += vctKey[i]->GetPersistenceLength(true);
		}

		if (lenKey > Configure::GetMaxKeyLength()) {
			throw utils::ErrorMsg(CORE_EXCEED_KEY_LENGTH, {std::to_string(lenKey)});
		}

		uint16_t lenVal = (bPri ? 0 : valVarNum * sizeof(uint16_t));
		uint16_t max_lenVal = Configure::GetMaxRecordLength() - lenKey - sizeof(uint16_t) * 3;
		for (i = 0; i < vctVal.size(); i++) {
			lenVal += vctVal[i]->GetPersistenceLength(!bPri);
			if (lenVal > max_lenVal) {
				lenVal -= vctVal[i]->GetPersistenceLength(!bPri);
				break;
			}
		}

		uint16_t indexOvfStart = (i < vctVal.size() ? i : UINT16_MAX);
		uint32_t sizeOverflow = 0;
		if (i < vctVal.size()) {
			for (; i < vctVal.size(); i++) {
				sizeOverflow += vctVal[i]->GetPersistenceLength(false);
			}

			sizeOverflow = (sizeOverflow + Configure::GetDiskClusterSize() - 1);
			sizeOverflow = sizeOverflow - sizeOverflow % Configure::GetDiskClusterSize();
			lenVal += sizeof(uint64_t) + sizeof(uint32_t);
		}

		int totalLen = lenKey + lenVal + sizeof(uint16_t) * 3;
		_bysVal = CachePool::ApplyBys(totalLen);
		*((uint16_t*)_bysVal) = totalLen;
		*((uint16_t*)(_bysVal + sizeof(uint16_t))) =indexOvfStart;
		*((uint16_t*)(_bysVal + sizeof(uint16_t) * 2)) = lenKey;

		uint16_t pos = (3 + keyVarNum) * sizeof(uint16_t);
		uint16_t lenPos = 3 * sizeof(uint16_t);

		for (i = 0; i < vctKey.size(); i++) {
			uint16_t len = vctKey[i]->WriteData(_bysVal + pos, true);
			if (!vctKey[i]->IsFixLength()) {
				*((uint16_t*)(_bysVal + lenPos)) = len;
				lenPos += sizeof(uint16_t);
			}

			pos += len;
		}

		pos = (3 + valVarNum) * sizeof(uint16_t) + lenKey;
		lenPos = 3 * sizeof(uint16_t) + lenKey;

		uint16_t ovfStart = (indexOvfStart == UINT16_MAX ? vctVal.size() : indexOvfStart);
		for (i = 0; i < ovfStart; i++) {
			if (!bPri) {
				int len = vctVal[i]->WriteData(_bysVal + pos, true);
				if (!vctVal[i]->IsFixLength()) {
					*((uint16_t*)(_bysVal + lenPos)) = len;
					lenPos += sizeof(uint16_t);
				}
				pos += len;
			}	else {
				pos += vctVal[i]->WriteData(_bysVal + pos, false);
			}
		}

		if (indexOvfStart != UINT16_MAX) {
			PageFile* ovf = indexTree->GetOverflowFile();

			if (oldSizeOverflow < sizeOverflow) {
				offset = ovf->GetOffsetAddLength(sizeOverflow);
			}
			else {
				sizeOverflow = oldSizeOverflow;
			}

			*((uint64_t*)(_bysVal + pos)) = offset;
			*((uint32_t*)(_bysVal + pos + sizeof(uint64_t))) = sizeOverflow;
			ovf->WriteDataValue(vctVal, indexOvfStart, offset);
		}
	}

	LeafRecord::~LeafRecord() {
		if (_bSole) CachePool::ReleaseBys(_bysVal, GetTotalLength());
		CleanUndoRecord();
	}

	void LeafRecord::CleanUndoRecord() {
		_tranId = UINT64_MAX;
		LeafRecord* rec = _undoRecords;
		while (rec != nullptr) {
			LeafRecord* next = rec->_undoRecords;
			delete rec;
			rec = next;
		}
	}

	void LeafRecord::AddOldRecord(LeafRecord* old) {
		_undoRecords = old;
	}

	vector<IDataValue*>& LeafRecord::GetListKey() const {
		vector<IDataValue*>& vctKey = _indexTree->CloneKeys();
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t pos = (3 + keyVarNum) * sizeof(uint16_t);
		uint16_t lenPos = 3 * sizeof(uint16_t);

		for (uint16_t i = 0; i < vctKey.size(); i++) {
			uint16_t len = 0;
			if (!vctKey[i]->IsFixLength()) {
				len = *((uint16_t*)(_bysVal + lenPos));
				lenPos += sizeof(uint16_t);
			}

			pos += vctKey[i]->ReadData(_bysVal + pos, len);
		}

		return vctKey;
	}

	vector<IDataValue*>& LeafRecord::GetListValue() const {
		vector<IDataValue*>& vctVal = _indexTree->CloneValues();
		bool bPri = (_indexTree->GetHeadPage()->ReadIndexType() == IndexType::PRIMARY);
		uint16_t valVarNum = bPri ? 0 : _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
		uint16_t pos = GetKeyLength() + (3 + valVarNum) * sizeof(uint16_t);
		uint16_t lenPos = 3 * sizeof(uint16_t) + GetKeyLength();

		int fCount = (GetIndexOvfStart() != UINT16_MAX ? GetIndexOvfStart() : vctVal.size());
		for (uint16_t i = 0; i < fCount; i++) {
			uint16_t len = 0;
			if (!vctVal[i]->IsFixLength() && !bPri) {
				len = *((uint16_t*)(_bysVal + lenPos));
				lenPos += sizeof(uint16_t);
			}

			pos += vctVal[i]->ReadData(_bysVal + pos, len);
		}

		return vctVal;
	}

	void LeafRecord::GetListOverflow(vector<IDataValue*>& vctVal) const {
		uint16_t indexOvfStart = GetIndexOvfStart();
		if (UINT16_MAX == indexOvfStart) return;

		uint64_t offset = *((uint64_t*)(_bysVal + GetTotalLength() - 12));
		uint32_t ovfLen = *((uint32_t*)(_bysVal + GetTotalLength() - 4));

		PageFile* ovf = _indexTree->GetOverflowFile();
		ovf->ReadDataValue(vctVal, indexOvfStart, offset, ovfLen);
	}

	int LeafRecord::CompareTo(const RawRecord& other) const {
		const LeafRecord& lr = (const LeafRecord&)other;
		
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		int rt = utils::BytesCompare(_bysVal + (3 + keyVarNum) * sizeof(uint16_t), GetKeyLength() - keyVarNum * sizeof(uint16_t),
			lr._bysVal + (3 + keyVarNum) * sizeof(uint16_t), lr.GetKeyLength() - keyVarNum * sizeof(uint16_t));
		if (rt != 0) {
			return rt;
		}

		return utils::BytesCompare(_bysVal + GetKeyLength() + 3 * sizeof(uint16_t), GetValueLength(),
			lr._bysVal + lr.GetKeyLength() + 3 * sizeof(uint16_t), lr.GetValueLength());
	}

	int LeafRecord::CompareKey(const RawKey& key) const {
		int keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();

		return utils::BytesCompare(_bysVal + (3 + keyVarNum) * sizeof(uint16_t),
			GetKeyLength() - keyVarNum * sizeof(uint16_t),
			key.GetBysVal(), key.GetLength());
	}

	int LeafRecord::CompareKey(const RawRecord& other) const {
		int keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		return utils::BytesCompare(_bysVal + (3 + keyVarNum) * sizeof(uint16_t),
			GetKeyLength() - keyVarNum * sizeof(uint16_t),
			other.GetBysValue() + (3 + keyVarNum) * sizeof(uint16_t),
			other.GetKeyLength() - keyVarNum * sizeof(uint16_t));
	}

	RawKey* LeafRecord::GetKey() const {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		return new RawKey(_bysVal + (3 + keyVarNum) * sizeof(uint16_t), GetKeyLength() - keyVarNum * sizeof(uint16_t));
	}

	std::ostream& operator<< (std::ostream& os, const LeafRecord& lr) {
		vector<IDataValue*>& vctKey = lr.GetListKey();
		os << "TotalLen=" << lr.GetTotalLength() << "  Keys=";
		for (IDataValue* dv : vctKey) {
			os << *dv << "; ";
			delete dv;
		}

		vector<IDataValue*>& vctVal = lr.GetListValue();
		os << "  Values=";
		for (IDataValue* dv : vctVal) {
			os << *dv << "; ";
			delete dv;
		}

		return os;
	}
		
}