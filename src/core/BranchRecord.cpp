#include "BranchRecord.h"
#include "IndexTree.h"

namespace storage {
  const uint32_t BranchRecord::PAGE_ID_LEN = sizeof(uint64_t);

	BranchRecord::BranchRecord(IndexTree* indexTree, RawRecord* rec, long childPageId) {
		_parentPage = nullptr;
		_indexTree = indexTree;
		_bSole = true;

		uint16_t lenKey = rec->GetKeyLength();
		uint16_t lenVal = (_indexTree->GetHeadPage()->ReadIndexType() == IndexType::NON_UNIQUE ?
			rec->GetValueLength() : 0);
		uint16_t totalLen = lenKey + lenVal + PAGE_ID_LEN + 2 * sizeof(uint16_t);
		_bysVal = CachePool::ApplyBys(totalLen);

		*((uint16_t*)_bysVal) = totalLen;
		*((uint16_t*)(_bysVal + sizeof(uint16_t))) = lenKey;

		uint16_t offset = (typeid(rec) == typeid(BranchRecord)) ? 2 * sizeof(uint16_t) : 3 * sizeof(uint16_t);
		std::memcpy(rec->GetBysValue() + offset, _bysVal + 2 * sizeof(uint16_t), lenKey + lenVal);
		*((uint64_t*)(_bysVal + lenKey + lenVal + 2 * sizeof(uint16_t))) = childPageId;
	}

	BranchRecord::BranchRecord(const BranchRecord& src)
	{
		_bysVal = src._bysVal;
		_parentPage = src._parentPage;
		_indexTree = src._indexTree;
		_bSole = src._bSole;
	}

	BranchRecord::~BranchRecord() {
		if (_bSole) CachePool::ReleaseBys(_bysVal, GetTotalLength());
	}

	RawKey* BranchRecord::GetKey() const {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		return new RawKey(_bysVal + (2 + keyVarNum) * sizeof(uint16_t), GetKeyLength() - keyVarNum * sizeof(uint16_t));
	}

	vector<IDataValue*>& BranchRecord::GetListKey() const {
		vector<IDataValue*>& vct = _indexTree->CloneKeys();
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		uint16_t pos = (2 + keyVarNum) * sizeof(uint16_t);
		uint16_t lenPos = 2 * sizeof(uint16_t);

		for (int i = 0; i < vct.size(); i++) {
			uint16_t len = 0;
			if (!vct[i]->IsFixLength(vct[i]->GetDataType())) {
				len = *(uint16_t*)(_bysVal + lenPos);
				lenPos += sizeof(uint16_t);
			}

			pos += vct[i]->ReadData(_bysVal + pos, len);
		}

		return vct;
	}

	vector<IDataValue*>& BranchRecord::GetListValue() const {
		if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) {
			vector<IDataValue*> vct;
			return vct;
		}

		vector<IDataValue*>& vct = _indexTree->CloneValues();
		uint16_t valVarNum = _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
		uint16_t pos = GetKeyLength() + (2 + valVarNum) * sizeof(uint16_t);
		uint16_t lenPos = 2 * sizeof(uint16_t) + GetKeyLength();

		for (int i = 0; i < vct.size(); i++) {
			int len = 0;
			if (!vct[i]->IsFixLength(vct[i]->GetDataType())) {
				len = *(uint16_t*)(_bysVal + lenPos);
				lenPos += sizeof(uint16_t);
			}

			pos += vct[i]->ReadData(_bysVal + pos, len);
		}

		return vct;
	}

	int BranchRecord::CompareTo(const RawRecord& other) const {
		BranchRecord& br = (BranchRecord&)other;
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		int rt = utils::BytesCompare(_bysVal + (2 + keyVarNum) * sizeof(uint16_t),
						GetKeyLength() - keyVarNum * sizeof(uint16_t),
						br._bysVal + (2 + keyVarNum) * sizeof(uint16_t),
						br.GetKeyLength() - keyVarNum * sizeof(uint16_t));
		if (rt != 0) return rt;

		if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) return 0;

		return utils::BytesCompare(_bysVal + GetKeyLength() + 2 * sizeof(uint16_t), GetValueLength(),
						br._bysVal + br.GetKeyLength() + 2 * sizeof(uint16_t), br.GetValueLength());
	}

	int BranchRecord::CompareKey(const RawKey& key) const {
		int keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();

		return utils::BytesCompare(_bysVal + (2 + keyVarNum) * sizeof(uint16_t),
					GetKeyLength() - keyVarNum * sizeof(uint16_t),
					key.GetBysVal(), key.GetLength());
	}

	int BranchRecord::CompareKey(const RawRecord& other) const {
		uint16_t keyVarNum = _indexTree->GetHeadPage()->ReadKeyVariableFieldCount();
		return utils::BytesCompare(_bysVal + (2 + keyVarNum) * sizeof(uint16_t),
			GetKeyLength() - keyVarNum * sizeof(uint16_t),
			other.GetBysValue() + (2 + keyVarNum) * sizeof(uint16_t),
			other.GetKeyLength() - keyVarNum * sizeof(uint16_t));
	}

	bool BranchRecord::EqualPageId(const BranchRecord& br) const {
		return (utils::BytesCompare(_bysVal + GetKeyLength() + GetValueLength() + 2 * sizeof(uint16_t), sizeof(uint64_t),
			br._bysVal + br.GetKeyLength() + br.GetValueLength() + 2 * sizeof(uint16_t), sizeof(uint64_t)) == 0);
	}

	std::ostream& operator<< (std::ostream& os, const BranchRecord& br) {
		vector<IDataValue*>& vctKey = br.GetListKey();
		os << "TotalLen=" << br.GetTotalLength() << "  Keys=";
		for (IDataValue* dv : vctKey) {
			os << *dv << "; ";
			delete dv;
		}

		if (br._indexTree->GetHeadPage()->ReadIndexType() == IndexType::NON_UNIQUE) {
			vector<IDataValue*>& vctVal = br.GetListValue();
			os << "  Values=";
			for (IDataValue* dv : vctVal) {
				os << *dv << "; ";
				delete dv;
			}

			os << "  ChildPageId=" << br.GetChildPageId();
		}

		return os;
	}
}