#include "BranchPage.h"
#include "BranchRecord.h"
#include "../pool/StoragePool.h"
#include "IndexTree.h"
#include  <shared_mutex>

namespace storage {
  const uint16_t BranchPage::DATA_BEGIN_OFFSET = 16;
  const uint16_t BranchPage::MAX_DATA_LENGTH = 
    (uint16_t)(Configure::GetCachePageSize() - DATA_BEGIN_OFFSET - sizeof(uint64_t));

  void BranchPage::ReadPage() {
    CachePage::ReadPage();

    ClearRecords();
    _recordNum = ReadShort(NUM_RECORD_OFFSET);
    _totalDataLength = ReadShort(TOTAL_DATA_LENGTH_OFFSET);
    _parentPageId = ReadLong(PARENT_PAGE_POINTER_OFFSET);
    _dtPageLastUpdate = GetMsFromEpoch();
  }

  void BranchPage::WritePage() {
    CachePage::WritePage();
  }

  void BranchPage::LoadRecords() {
    ClearRecords();
    uint16_t pos = DATA_BEGIN_OFFSET;

    for (uint16_t i = 0; i < _recordNum; i++) {
      BranchRecord* rr = new BranchRecord(this, _bysPage + *((uint16_t*)(_bysPage + pos)));
      _vctRecord.push_back(rr);
      pos += sizeof(uint16_t);
    }
  }

  BranchRecord* BranchPage::DeleteRecord(uint16_t index) {
    assert(index < _recordNum && _vctRecord.size() > 0);
    BranchRecord* br = (BranchRecord*) _vctRecord[index];
    _vctRecord.erase(_vctRecord.begin() + index);
    _totalDataLength -= br->GetTotalLength();
    _recordNum--;
    _bRecordUpdate = true;
    _bDirty = true;
    return br;
  }

  bool BranchPage::SaveRecord() {
    shared_lock<utils::SharedSpinMutex> lock(_rwLock);

    if (_totalDataLength > MAX_DATA_LENGTH) return false;

    if (_bRecordUpdate) {
      Byte* tmp = _bysPage;
      _bysPage = CachePool::ApplyPage();
      uint16_t pos = DATA_BEGIN_OFFSET + _recordNum * sizeof(uint16_t);

      for (int i = 0; i < _vctRecord.size(); i++) {
        WriteShort(DATA_BEGIN_OFFSET + sizeof(uint16_t) * i, pos);
        BranchRecord* rr = (BranchRecord*)_vctRecord[i];
        rr->SaveData(_bysPage + pos);
        pos += rr->GetTotalLength();
        delete rr;
      }

      _vctRecord.clear();
      _bysPage[PAGE_LEVEL_OFFSET] = tmp[PAGE_LEVEL_OFFSET];
      CachePool::ReleasePage(tmp);
    }

    WriteLong(PARENT_PAGE_POINTER_OFFSET, _parentPageId);
    WriteShort(TOTAL_DATA_LENGTH_OFFSET, _totalDataLength);
    WriteShort(NUM_RECORD_OFFSET, _recordNum);
    StoragePool::WriteCachePage(this);
    _bRecordUpdate = false;
    return true;
  }

  void BranchPage::InsertRecord(BranchRecord* rr) {
    if (_recordNum > 0 && _vctRecord.size() == 0) LoadRecords();

    uint32_t index = SearchRecord(*rr);
    _totalDataLength += rr->GetTotalLength() + sizeof(uint16_t);
    rr->SetParentPage(this);
    _vctRecord.insert(_vctRecord.begin() + index, rr);
    _recordNum++;
    _bDirty = true;
    _bRecordUpdate = true;
  }

  BranchRecord* BranchPage::DeleteRecord(BranchRecord* rr) {
    if (_vctRecord.size() == 0) {
      LoadRecords();
    }

    uint32_t index = SearchRecord(*rr);
    BranchRecord* br = (BranchRecord*)_vctRecord[index];
    _vctRecord.erase(_vctRecord.begin() + index);
    _totalDataLength -= br->GetTotalLength();
    _recordNum--;
    _bRecordUpdate = true;
    _bDirty = true;
    return br;
  }

  bool BranchPage::AddRecord(BranchRecord* rr) {
    if (_totalDataLength > MAX_DATA_LENGTH * LOAD_FACTOR
      || _totalDataLength + rr->GetTotalLength() + sizeof(uint16_t) > MAX_DATA_LENGTH) {
      return false;
    }

    _totalDataLength += rr->GetTotalLength() + sizeof(uint16_t);
    rr->SetParentPage(this);
    _vctRecord.push_back(rr);
    _recordNum++;
    _bDirty = true;
    _bRecordUpdate = true;

    return true;
  }

  uint32_t BranchPage::SearchRecord(const BranchRecord& rr) {
    if (_recordNum == 0) return 0;

    uint32_t start = 0;
    uint32_t end = _recordNum - 1;
    int hr = (_vctRecord.size() > 0 ? _vctRecord[end]->CompareTo(rr) : CompareTo(end, rr));
    if (hr <= 0) return end;

    end--;
    while (true) {
      if (start > end) { return start; }

      int middle = (start + end) / 2;
      hr = (_vctRecord.size() > 0 ? _vctRecord[middle]->CompareTo(rr) : CompareTo(middle, rr));
      if (hr < 0) { start = middle + 1; }
      else if (hr > 0) { end = middle - 1; }
      else { return middle; }
    }
  }

  uint32_t BranchPage::SearchKey(const RawKey& key) {
    	if (_recordNum == 0) return 0;

    	bool bUnique = (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE);
      uint32_t start = 0;
      uint32_t end = _recordNum - 1;
    	int hr = (_vctRecord.size() > 0 ? _vctRecord[end]->CompareKey(key) : CompareTo(end, key));
    	if (hr < 0 || (hr == 0 && bUnique)) {
    		return end;
    	}

    	end--;

    	while (true) {
    		if (start > end) {
    			return start;
    		}

        uint32_t middle = (start + end) / 2;
    		hr = (_vctRecord.size() > 0 ? _vctRecord[middle]->CompareKey(key) : CompareTo(middle, key));
    		if (hr < 0) {
    			start = middle + 1;
    		}
    		else if (hr > 0) {
    			end = middle - 1;
    		}
    		else {
    			if (!bUnique && middle > start && (_vctRecord.size() > 0 ? _vctRecord[middle - 1]->CompareKey(key) == 0
    				: CompareTo(middle - 1, key) == 0)) {
    				end = middle - 1;
    			}
    			else {
    				return middle;
    			}
    		}
    	}
    }

  int BranchPage::CompareTo(uint32_t recPos, const BranchRecord& rr) {
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
    uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * sizeof(uint16_t));
    uint32_t lenKey = ReadShort(start + sizeof(uint16_t));

    int rt = utils::BytesCompare(_bysPage + start + (2 + keyVarNum) * sizeof(uint16_t),
      lenKey - keyVarNum * sizeof(uint16_t),
      rr.GetBysValue() + (2 + keyVarNum) * sizeof(uint16_t),
      rr.GetKeyLength() - keyVarNum * sizeof(uint16_t));

    if (rt != 0) {
      return rt;
    }

    if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) {
      return 0;
    }

    int lenTotal = ReadShort(start);
    int lenVal = lenTotal - lenKey - BranchRecord::PAGE_ID_LEN - sizeof(uint16_t) * 2;
    return utils::BytesCompare(_bysPage + start + lenKey + 2 * sizeof(uint16_t), lenVal,
      rr.GetBysValue() + rr.GetKeyLength() + 2 * sizeof(uint16_t), rr.GetValueLength());
  }

  int BranchPage::CompareTo(uint32_t recPos, const RawKey& key) {
    uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
    uint32_t start = ReadShort(DATA_BEGIN_OFFSET + recPos * sizeof(uint16_t));
    uint32_t lenKey = ReadShort(start + sizeof(uint16_t));

    return utils::BytesCompare(_bysPage + start + (2 + keyVarNum) * sizeof(uint16_t),
      lenKey - keyVarNum * sizeof(uint16_t),
      key.GetBysVal(), key.GetLength());
  }

  BranchRecord* BranchPage::GetRecordByPos(uint32_t pos) {
    if (pos < 0 || pos >= _recordNum) {
      return nullptr;
    }

    if (_vctRecord.size() == 0) {
      uint16_t offset = ReadShort(DATA_BEGIN_OFFSET + pos * sizeof(uint16_t));
      return new BranchRecord(this, _bysPage + offset);
    }
    else {
      return ((BranchRecord*)_vctRecord[pos])->ReferenceRecord();
    }
  }
}