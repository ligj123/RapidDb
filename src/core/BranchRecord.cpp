#include "BranchRecord.h"
#include "../dataType/DataValueFactory.h"
#include "BranchPage.h"
#include "IndexTree.h"
#include <memory>

namespace storage {
const uint32_t BranchRecord::PAGE_ID_LEN = sizeof(PageID);

BranchRecord::BranchRecord(IndexType type, Byte *bys)
    : RawRecord(bys, false, type) {}

BranchRecord::BranchRecord(IndexType type, RawRecord *rec, uint32_t childPageId)
    : RawRecord(nullptr, true, type) {
  uint16_t lenKey = rec->GetKeyLength();
  uint16_t lenVal = (type == IndexType::NON_UNIQUE ? rec->GetValueLength() : 0);
  uint16_t totalLen = lenKey + lenVal + PAGE_ID_LEN + UI16_2_LEN;
  _bysVal = CachePool::Apply(totalLen);

  *((uint16_t *)_bysVal) = totalLen;
  *((uint16_t *)(_bysVal + UI16_LEN)) = lenKey;

  BytesCopy(_bysVal + UI16_2_LEN, rec->GetBysValue() + UI16_2_LEN,
            lenKey + lenVal);
  *((uint32_t *)(_bysVal + lenKey + lenVal + UI16_2_LEN)) = childPageId;
}

int BranchRecord::CompareTo(const RawRecord &rr, IndexType type) const {
  if (type != IndexType::NON_UNIQUE) {
    return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength(),
                        rr.GetBysValue() + UI16_2_LEN, rr.GetKeyLength());
  } else {
    return BytesCompare(_bysVal + UI16_2_LEN,
                        GetTotalLength() - UI16_2_LEN - PAGE_ID_LEN,
                        rr.GetBysValue() + UI16_2_LEN,
                        rr.GetTotalLength() - UI16_2_LEN - PAGE_ID_LEN);
  }
}

int BranchRecord::CompareKey(const RawKey &key) const {
  return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength(), key.GetBysVal(),
                      key.GetLength());
}

int BranchRecord::CompareKey(const RawRecord &rr) const {
  return BytesCompare(_bysVal + UI16_2_LEN, GetKeyLength(),
                      rr.GetBysValue() + UI16_2_LEN, rr.GetKeyLength());
}

bool BranchRecord::EqualPageId(const BranchRecord &br) const {
  return (BytesCompare(_bysVal + GetTotalLength() - PAGE_ID_LEN, PAGE_ID_LEN,
                       br._bysVal + br.GetTotalLength() - PAGE_ID_LEN,
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

  if (br._indexType == IndexType::NON_UNIQUE) {
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
