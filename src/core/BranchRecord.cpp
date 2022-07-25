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

int BranchRecord::CompareTo(const RawRecord &rr) const {
  assert(typeid(rr) == typeid(BranchRecord));
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
  os << "TotalLen=" << br.GetTotalLength() << "  Keys=";

  os << std::uppercase << std::hex << std::setfill('0') << "0x";
  Byte *bys = br._bysVal + UI16_2_LEN;
  for (uint32_t i = 0; i < br.GetKeyLength(); i++) {
    os << std::setw(2) << bys++;
    if (i % 4 == 0)
      os << ' ';
  }

  if (br._indexTree->GetHeadPage()->ReadIndexType() == IndexType::NON_UNIQUE) {
    os << "\nValues=";
    for (uint32_t i = 0; i < br.GetValueLength(); i++) {
      os << std::setw(2) << bys++;
      if (i % 4 == 0)
        os << ' ';
    }
  }

  os << "\nChildPageId=" << br.GetChildPageId();
  return os;
}
} // namespace storage
