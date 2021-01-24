#include "LeafPage.h"
#include "BranchPage.h"
#include "IndexTree.h"
#include "HeadPag.h"
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
		IndexPage(indexTree, pageId, 0, parentPageId) {

		_prevPageId = HeadPage::NO_PREV_PAGE_POINTER;
		_nextPageId = HeadPage::NO_NEXT_PAGE_POINTER;
	}

	LeafPage::LeafPage(IndexTree* indexTree, uint64_t pageId) :
		IndexPage(indexTree, pageId) { }

	void LeafPage::LoadRecords() {
		CleanRecord();

		uint16_t pos = (uint16_t)(DATA_BEGIN_OFFSET + _recordNum * sizeof(uint16_t));

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

	bool LeafPage::SaveRecord() {
		unique_lock<utils::SharedSpinMutex> lock;
			if (_totalDataLength > MAX_DATA_LENGTH) return false;

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
					lr->SaveData(_bysPage + pos);
					pos += lr->GetTotalLength();
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
			StoragePool::WriteCachePage(this);
			_bRecordUpdate = false;
			return true;
	}

	void LeafPage::InsertRecord(LeafRecord* lr) {
		if (_recordNum > 0 && _vctRecord.size() == 0) {
			LoadRecords();
		}

		bool bUnique = (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
		int pos;
		if (bUnique) {
			RawKey* key = lr->GetKey();
			pos = SearchKey(*key, false);
			delete key;
		}
		else {
			pos = SearchRecord(*lr, false);
		}

		LeafRecord* rr = (pos >= _vctRecord.size() ? nullptr : (LeafRecord*)_vctRecord[pos]);
		if (rr != nullptr) {
			if (bUnique && lr->CompareKey(*rr) == 0) {
				throw utils::ErrorMsg(CORE_REPEATED_RECORD, {});
			}
			else if (lr->CompareTo(*rr)) {
				throw new utils::ErrorMsg(CORE_REPEATED_RECORD, {});
			}
		}

		_totalDataLength += lr->GetTotalLength() + sizeof(uint16_t);
		lr->SetParentPage(this);
		_vctRecord.insert(_vctRecord.begin() + pos, lr);
		_recordNum++;
		_bDirty = true;
		_bRecordUpdate = true;
		_indexTree->GetHeadPage()->GetAndAddTotalRecordNum(1);
		PageDividePool::AddCachePage(this);
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
		_indexTree->GetHeadPage()->GetAndIncTotalPageNum();

		return true;
	}

	LeafRecord* LeafPage::GetRecord(const RawKey& key) {
		if (_recordNum == 0) {
			return nullptr;
		}

		uint32_t index = SearchKey(key, true);
		if (index >= _recordNum) {
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

	vector<LeafRecord*>* LeafPage::GetRecords(const RawKey& key) {
		vector<LeafRecord*>* vct = new vector<LeafRecord*>(_recordNum);
		if (_recordNum == 0) return vct;

		uint32_t pos = SearchKey(key, true);
		if (_vctRecord.size() == 0) {
			for (uint16_t i = pos; i < _recordNum; i++) {
				uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + i * sizeof(uint16_t));
				if (i == pos) {
					vct->push_back(new LeafRecord(this, _bysPage + offset));
				}
				else if (CompareTo(pos, key) == 0) {
					vct->push_back(new LeafRecord(this, _bysPage + offset));
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
					vct->push_back(rr);
				}
				else if (rr->CompareKey(key) == 0) {
					vct->push_back(rr);
				}
				else {
					rr->ReleaseRecord();
					break;
				}
			}
		}

		return vct;
	}

	uint32_t LeafPage::SearchRecord(const LeafRecord& rr, bool bEqual) {
		if (_recordNum == 0) {
			if (bEqual) {
				return UINT32_MAX;
			}
			else {
				return 0;
			}
		}

		uint32_t start = 0;
		uint32_t end = (uint32_t)_vctRecord.size() - 1;
		int hr = (_vctRecord.size() > 0 ? _vctRecord[end]->CompareTo(rr) : CompareTo(end, rr));
		if (hr < 0) {
			if (bEqual) {
				return UINT32_MAX;
			}
			else {
				return end + 1;
			}
		}
		else if (hr == 0) {
			return end;
		}
		else {
			end--;
		}

		while (true) {
			if (start > end) {
				if (bEqual) {
					return UINT32_MAX;
				}
				else {
					return start;
				}
			}

			int middle = (start + end) / 2;
			if (_vctRecord.size() > 0) {
				LeafRecord* currRr = (LeafRecord*)_vctRecord[middle];
				hr = currRr->CompareTo(rr);
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

	uint32_t LeafPage::SearchKey(const RawKey& key, bool bEqual) {
		if (_recordNum == 0) {
			if (bEqual) {
				return UINT32_MAX;
			}
			else {
				return 0;
			}
		}

		uint32_t start = 0;
		uint32_t end = (uint32_t)_vctRecord.size() - 1;
		bool bUnique = (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
		if (bUnique) {
			int hr = 0;
			if (_vctRecord.size() > 0) {
				hr = _vctRecord[end]->CompareKey(key);
			}
			else {
				hr = CompareTo(end, key);
			}

			if (hr < 0) {
				if (bEqual) {
					return UINT32_MAX;
				}
				else {
					return end + 1;
				}
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
				if (bEqual) {
					return UINT32_MAX;
				}
				else {
					return start;
				}
			}

			uint32_t middle = (start + end) / 2;
			int hr = 0;
			if (_vctRecord.size() > 0) {
				RawRecord* currRr = _vctRecord[middle];
				hr = currRr->CompareKey(key);
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
					_vctRecord[middle - 1]->CompareKey(key) == 0 : CompareTo(middle - 1, key) == 0)) {
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
			return ((LeafRecord*)_vctRecord[_vctRecord.size()])->ReferenceRecord();
		}
		else {
			uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + (_recordNum - 1) * sizeof(uint16_t));
			return new LeafRecord(this, _bysPage + offset);
		}
	}

	vector<LeafRecord*>* LeafPage::GetAllRecords() {
		vector<LeafRecord*>* vct = new vector<LeafRecord*>(_recordNum);
		if (_vctRecord.size() == 0) { LoadRecords(); }
		for (RawRecord* rr : _vctRecord) {
			vct->push_back(((LeafRecord*)rr)->ReferenceRecord());
		}

		return vct;
	}

	int LeafPage::CompareTo(uint32_t recPos, const RawKey& key) {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * sizeof(uint16_t));
		uint16_t lenKey = ReadShort(start + 2 * sizeof(uint16_t));

		return utils::BytesCompare(_bysPage + start + (3 + keyVarNum) * sizeof(uint16_t),
			lenKey - keyVarNum * sizeof(uint16_t),
			key.GetBysVal(), key.GetLength());
	}

	int LeafPage::CompareTo(uint32_t recPos, const LeafRecord& rr) {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * sizeof(uint16_t));
		uint16_t lenKey = ReadShort(start + 2 * sizeof(uint16_t));

		int hr = utils::BytesCompare(_bysPage + start + (3 + keyVarNum) * sizeof(uint16_t),
			lenKey - keyVarNum * sizeof(uint16_t),
			rr.GetBysValue() + (3 + keyVarNum) * sizeof(uint16_t),
			rr.GetKeyLength() - keyVarNum * sizeof(uint16_t));
		if (hr != 0) { return hr; }

		uint16_t valVarNum = _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
		uint16_t lenTotal = ReadShort(start);
		uint16_t lenVal = lenTotal - lenKey - sizeof(uint16_t) * 3;
		return utils::BytesCompare(_bysPage + start + lenKey + (3 + valVarNum) * sizeof(uint16_t),
			lenVal - valVarNum * sizeof(uint16_t),
			rr.GetBysValue() + rr.GetKeyLength() + (3 + valVarNum) * sizeof(uint16_t),
			rr.GetValueLength() - valVarNum * sizeof(uint16_t));
	}
}