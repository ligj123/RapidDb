#include "BranchRecord.h"
#include "../dataType/DataValueFactory.h"
#include "BranchPage.h"
#include "IndexTree.h"
#include <memory>

namespace storage {
const uint32_t BranchRecord::PAGE_ID_LEN = sizeof(PageID);

BranchRecord::BranchRecord(BranchPage *parentPage, Byte *bys)
    : RawRecord(parentPage->GetIndexTree(), parentPage, bys, false) {}

BranchRecord::BranchRecord(IndexTree *indexTree, RawRecord *rec,
                           uint32_t childPageId)
    : RawRecord(indexTree, nullptr, nullptr, true) {
  uint16_t lenKey = rec->GetKeyLength();
  uint16_t lenVal =
      (_indexTree->GetHeadPage()->ReadIndexType() == IndexType::NON_UNIQUE
           ? rec->GetValueLength()
           : 0);
  uint16_t totalLen = lenKey + lenVal + PAGE_ID_LEN + UI16_2_LEN;
  _bysVal = CachePool::Apply(totalLen);

  *((uint16_t *)_bysVal) = totalLen;
  *((uint16_t *)(_bysVal + UI16_LEN)) = lenKey;

  BytesCopy(_bysVal + UI16_2_LEN, rec->GetBysValue() + UI16_2_LEN,
            lenKey + lenVal);
  *((uint32_t *)(_bysVal + lenKey + lenVal + UI16_2_LEN)) = childPageId;
}

RawKey *BranchRecord::GetKey() const {
  return new RawKey(_bysVal + _indexTree->GetKeyOffset(),
                    GetKeyLength() - _indexTree->GetKeyVarLen());
}

void BranchRecord::GetListKey(VectorDataValue &vct) const {
  _indexTree->CloneKeys(vct);
  uint16_t pos = _indexTree->GetKeyOffset();
  uint16_t lenPos = UI16_2_LEN;

  for (int i = 0; i < vct.size(); i++) {
    uint16_t len = 0;
    if (!vct[i]->IsFixLength()) {
      len = *(uint16_t *)(_bysVal + lenPos);
      lenPos += sizeof(uint16_t);
    }

    pos += vct[i]->ReadData(_bysVal + pos, len);
  }
}

void BranchRecord::GetListValue(VectorDataValue &vct) const {
  if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) {
    return;
  }

  _indexTree->CloneValues(vct);
  uint16_t valVarNum = _indexTree->GetHeadPage()->ReadValueVariableFieldCount();
  uint16_t pos = GetKeyLength() + _indexTree->GetKeyOffset();
  uint16_t lenPos = UI16_2_LEN + GetKeyLength();

  for (int i = 0; i < vct.size(); i++) {
    int len = 0;
    if (!vct[i]->IsFixLength()) {
      len = *(uint16_t *)(_bysVal + lenPos);
      lenPos += sizeof(uint16_t);
    }

    pos += vct[i]->ReadData(_bysVal + pos, len);
  }
}

int BranchRecord::CompareTo(const RawRecord &rr) const {
  if (_indexTree->GetHeadPage()->ReadIndexType() != IndexType::NON_UNIQUE) {
    return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                        GetKeyLength() - _indexTree->GetKeyVarLen(),
                        rr.GetBysValue() + _indexTree->GetKeyOffset(),
                        rr.GetKeyLength() - _indexTree->GetKeyVarLen());
  } else {
    return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                        GetTotalLength() - _indexTree->GetKeyOffset(),
                        rr.GetBysValue() + _indexTree->GetKeyOffset(),
                        rr.GetTotalLength() - _indexTree->GetKeyOffset());
  }
}

int BranchRecord::CompareKey(const RawKey &key) const {
  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                      GetKeyLength() - _indexTree->GetKeyVarLen(),
                      key.GetBysVal(), key.GetLength());
}

int BranchRecord::CompareKey(const RawRecord &rr) const {
  return BytesCompare(_bysVal + _indexTree->GetKeyOffset(),
                      GetKeyLength() - _indexTree->GetKeyVarLen(),
                      rr.GetBysValue() + _indexTree->GetKeyOffset(),
                      rr.GetKeyLength() - _indexTree->GetKeyVarLen());
}

bool BranchRecord::EqualPageId(const BranchRecord &br) const {
  return (BytesCompare(_bysVal + GetKeyLength() + GetValueLength() + UI16_2_LEN,
                       PAGE_ID_LEN,
                       br._bysVal + br.GetKeyLength() + br.GetValueLength() +
                           UI16_2_LEN,
                       PAGE_ID_LEN) == 0);
}

std::ostream &operator<<(std::ostream &os, const BranchRecord &br) {
  VectorDataValue vctKey;
  br.GetListKey(vctKey);
  os << "TotalLen=" << br.GetTotalLength() << "  Keys=";
  for (IDataValue *dv : vctKey) {
    os << *dv << "; ";
  }

  if (br._indexTree->GetHeadPage()->ReadIndexType() == IndexType::NON_UNIQUE) {
    VectorDataValue vctVal;
    br.GetListValue(vctVal);
    os << "  Values=";
    for (IDataValue *dv : vctVal) {
      os << *dv << "; ";
    }
  }

  os << "  ChildPageId=" << br.GetChildPageId();
  return os;
}
} // namespace storage
