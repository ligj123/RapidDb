#include "LeafPage.h"
#include "BranchPage.h"
#include "IndexTree.h"
#include "HeadPage.h"
#include "LeafRecord.h"
#include "../pool/StoragePool.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../pool/PageDividePool.h"
#include "LeafRecord.h"

namespace storage {
	uint16_t LeafPage::PREV_PAGE_POINTER_OFFSET = 16;
	uint16_t LeafPage::NEXT_PAGE_POINTER_OFFSET = 24;
	uint16_t LeafPage::DATA_BEGIN_OFFSET = 32;
	uint16_t LeafPage::MAX_DATA_LENGTH = 
		(uint16_t)(Configure::GetCachePageSize() - LeafPage::DATA_BEGIN_OFFSET - sizeof(uint64_t));

	LeafPage::LeafPage(IndexTree* indexTree, uint64_t pageId, uint64_t parentPageId) :
		IndexPage(indexTree, pageId, 0, parentPageId), 
		_prevPageId(HeadPage::NO_PREV_PAGE_POINTER),
		_nextPageId(HeadPage::NO_NEXT_PAGE_POINTER) {	}

	LeafPage::LeafPage(IndexTree* indexTree, uint64_t pageId) :
		IndexPage(indexTree, pageId),
		_prevPageId(HeadPage::NO_PREV_PAGE_POINTER),
		_nextPageId(HeadPage::NO_NEXT_PAGE_POINTER) { }

	LeafPage::~LeafPage() {
		CleanRecord();
	}

	void LeafPage::Init() {
		IndexPage::Init();
		_prevPageId = ReadLong(PREV_PAGE_POINTER_OFFSET);
		_nextPageId = ReadLong(NEXT_PAGE_POINTER_OFFSET);
	}
	void LeafPage::LoadRecords() {
		CleanRecord();

		uint16_t pos = DATA_BEGIN_OFFSET;
		for (uint16_t i = 0; i < _recordNum; i++) {
			LeafRecord* rr = new LeafRecord(this, _bysPage + *((uint16_t*)(_bysPage + pos)));
			_vctRecord.push_back(rr);
			pos += sizeof(uint16_t);
		}
	}

	void LeafPage::CleanRecord() {
		for (RawRecord* lr : _vctRecord) {
			((LeafRecord*)lr)->ReleaseRecord();
		}

		_vctRecord.clear();
	}

	bool LeafPage::SaveRecords() {
		if (_totalDataLength > MAX_DATA_LENGTH) return false;

		unique_lock<SpinMutex> lock(_pageLock, try_to_lock);
		if (!lock.owns_lock()) return false;

		if (_bRecordUpdate) {
			Byte* tmp = _bysPage;
			_bysPage = CachePool::ApplyPage();
			uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * sizeof(uint16_t));
			uint16_t index = 0;

			for (uint16_t i = 0; i < _vctRecord.size(); i++) {
				LeafRecord* lr = (LeafRecord*)_vctRecord[i];
				if (lr->GetActionType() == ActionType::DELETE) {
					continue;
				}

				WriteShort(DATA_BEGIN_OFFSET + sizeof(uint16_t) * index, pos);
				pos += lr->SaveData(_bysPage + pos);
				index++;
			}

			CleanRecord();

			_bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
			CachePool::ReleasePage(tmp);
		}

		WriteLong(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
		WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
		WriteLong(PREV_PAGE_POINTER_OFFSET, _prevPageId);
		WriteLong(NEXT_PAGE_POINTER_OFFSET, _nextPageId);
		WriteShort(NUM_RECORD_OFFSET, _recordNum);
		_bRecordUpdate = false;
		_bDirty = true;

		StoragePool::WriteCachePage(this);
		return true;
	}

	utils::ErrorMsg* LeafPage::InsertRecord(LeafRecord* lr, int32_t pos) {
		if (_recordNum > 0 && _vctRecord.size() == 0) {
			LoadRecords();
		}

		bool bUnique = (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
		if (pos < 0) {
			bool bFind;
			if (bUnique) {
				RawKey* key = lr->GetKey();
				pos = SearchKey(*key, bFind);
				delete key;
			}
			else {
				pos = SearchRecord(*lr, bFind);
			}

			if (bFind) {
				return new utils::ErrorMsg(CORE_REPEATED_RECORD, {});
			}
		}
		else if (pos > _recordNum) {
			pos = _recordNum;
		}

		_totalDataLength += lr->GetTotalLength() + sizeof(uint16_t);
		lr->SetParentPage(this);
		_vctRecord.insert(_vctRecord.begin() + pos, lr);
		_recordNum++;
		_bDirty = true;
		_bRecordUpdate = true;
		_indexTree->GetHeadPage()->GetAndIncTotalRecordCount();
		PageDividePool::AddCachePage(this);

		return nullptr;
	}

	bool LeafPage::AddRecord(LeafRecord* lr) {
		if (_totalDataLength > MAX_DATA_LENGTH * LOAD_FACTOR
			|| _totalDataLength + lr->GetTotalLength() + sizeof(uint16_t) > MAX_DATA_LENGTH) {
			return false;
		}

		if (_recordNum > 0 && _vctRecord.size() == 0) {
			LoadRecords();
		}

		_totalDataLength += lr->GetTotalLength() + sizeof(uint16_t);
		lr->SetParentPage(this);
		_vctRecord.push_back(lr);
		_recordNum++;
		_bDirty = true;
		_bRecordUpdate = true;
		_indexTree->GetHeadPage()->GetAndIncTotalPageCount();

		return true;
	}

	void LeafPage::DeleteRecord(int32_t pos) {
		assert(pos >= 0 && pos < _recordNum);
	
	}

	void LeafPage::UpdateRecord(int32_t pos) {
		assert(pos >= 0 && pos < _recordNum);

	}

	LeafRecord* LeafPage::GetRecord(const RawKey& key) {
		if (_recordNum == 0) {
			return nullptr;
		}

		bool bFind;
		int32_t index = SearchKey(key, bFind);
		if (!bFind) {
			return nullptr;
		}

		if (_vctRecord.size() == 0) {
			int pos = ReadShort(DATA_BEGIN_OFFSET + index * sizeof(uint16_t));
			return new LeafRecord(this, _bysPage + pos);
		}
		else {
			return ((LeafRecord*)_vctRecord[index])->ReferenceRecord();
		}
	}

	void LeafPage::GetRecords(const RawKey& key, VectorLeafRecord& vct) {
		if (vct.size() > 0) vct.RemoveAll();
		if (_recordNum == 0) return;

		bool bFind;
		uint32_t pos = SearchKey(key, bFind);
		if (!bFind) return;
		vct.reserve(_recordNum);

		if (_vctRecord.size() == 0) {
			for (uint16_t i = pos; i < _recordNum; i++) {
				uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + i * sizeof(uint16_t));
				if (i == pos) {
					vct.push_back(new LeafRecord(this, _bysPage + offset));
				}
				else if (CompareTo(i, key) == 0) {
					vct.push_back(new LeafRecord(this, _bysPage + offset));
				}
				else {
					break;
				}
			}
		}
		else {
			for (uint32_t i = pos; i < _vctRecord.size(); i++) {
				LeafRecord* rr = ((LeafRecord*)_vctRecord[i])->ReferenceRecord();
				if (i == pos) {
					vct.push_back(rr);
				}
				else if (rr->CompareKey(key) == 0) {
					vct.push_back(rr);
				}
				else {
					rr->ReleaseRecord();
					break;
				}
			}
		}
	}

	int32_t LeafPage::SearchRecord(const LeafRecord& rr, bool& bFind) {
		if (_recordNum == 0) {
			bFind = false;
			return 0;
		}
		 
		int32_t start = 0;
		int32_t end = _recordNum - 1;
		bFind = true;
		int hr = (_vctRecord.size() > 0 ? GetVctRecord(end)->CompareTo(rr) : CompareTo(end, rr));
		if (hr < 0) {
			bFind = false;
			return end + 1;
		}
		else if (hr == 0) {
			return end;
		}
		else {
			end--;
		}

		while (true) {
			if (start > end) {
				bFind = false;
				return start;
			}

			int middle = (start + end) / 2;
			if (_vctRecord.size() > 0) {
				hr = GetVctRecord(middle)->CompareTo(rr);
			}
			else {
				hr = CompareTo(middle, rr);
			}

			if (hr < 0) {
				start = middle + 1;
			}
			else if (hr > 0) {
				end = middle - 1;
			}
			else {
				return middle;
			}
		}
	}

	int32_t LeafPage::SearchKey(const RawKey& key, bool& bFind) {
		if (_recordNum == 0) {
			bFind = false;
				return 0;
		}

		int32_t start = 0;
		int32_t end = _recordNum - 1;
		bFind = true;
		bool bUnique = (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
		if (bUnique) {
			int hr = 0;
			if (_vctRecord.size() > 0) {
				hr = GetVctRecord(end)->CompareKey(key);
			}
			else {
				hr = CompareTo(end, key);
			}

			if (hr < 0) {
				bFind = false;
				return end + 1;
			}
			else if (hr == 0) {
				return end;
			}
			else {
				end--;
			}
		}

		while (true) {
			if (start > end) {
				bFind = false;
				return start;
			}

			int32_t middle = (start + end) / 2;
			int hr = 0;
			if (_vctRecord.size() > 0) {
				hr = GetVctRecord(middle)->CompareKey(key);
			}
			else {
				hr = CompareTo(middle, key);
			}

			if (hr < 0) {
				start = middle + 1;
			}
			else if (hr > 0) {
				end = middle - 1;
			}
			else {
				if (!bUnique && middle > start && (_vctRecord.size() > 0 ?
					GetVctRecord(middle - 1)->CompareKey(key) == 0 : CompareTo(middle - 1, key) == 0)) {
					end = middle - 1;
				}
				else {
					return middle;
				}
			}
		}
	}

	LeafRecord* LeafPage::GetLastRecord() {
		if (_recordNum == 0) return nullptr;
		if (_vctRecord.size() > 0) {
			return GetVctRecord(_recordNum - 1)->ReferenceRecord();
		}
		else {
			uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + (_recordNum - 1) * sizeof(uint16_t));
			return new LeafRecord(this, _bysPage + offset);
		}
	}

	void LeafPage::GetAllRecords(VectorLeafRecord& vct) {
		vct.RemoveAll();
		vct.reserve(_recordNum);
		if (_vctRecord.size() == 0) { LoadRecords(); }

		for (RawRecord* rr : _vctRecord) {
			vct.push_back(((LeafRecord*)rr)->ReferenceRecord());
		}
	}

	int LeafPage::CompareTo(uint32_t recPos, const RawKey& key) {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * sizeof(uint16_t));
		uint16_t lenKey = ReadShort(start + sizeof(uint16_t));

		return utils::BytesCompare(_bysPage + start + (2 + keyVarNum) * sizeof(uint16_t),
			lenKey - keyVarNum * sizeof(uint16_t),
			key.GetBysVal(), key.GetLength());
	}

	int LeafPage::CompareTo(uint32_t recPos, const LeafRecord& rr) {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * sizeof(uint16_t));
		uint16_t lenKey = ReadShort(start + sizeof(uint16_t));

		int hr = utils::BytesCompare(_bysPage + start + (2 + keyVarNum) * sizeof(uint16_t),
			lenKey - keyVarNum * sizeof(uint16_t),
			rr.GetBysValue() + (2 + keyVarNum) * sizeof(uint16_t),
			rr.GetKeyLength() - keyVarNum * sizeof(uint16_t));
		if (hr != 0) { return hr; }

		uint16_t valVarNum = _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
		uint16_t lenTotal = ReadShort(start);
		uint16_t lenVal = lenTotal - lenKey - sizeof(uint16_t) * 2;
		return utils::BytesCompare(_bysPage + start + lenKey + (2 + valVarNum) * sizeof(uint16_t),
			lenVal - valVarNum * sizeof(uint16_t),
			rr.GetBysValue() + rr.GetKeyLength() + (2 + valVarNum) * sizeof(uint16_t),
			rr.GetValueLength() - valVarNum * sizeof(uint16_t));
	}
}