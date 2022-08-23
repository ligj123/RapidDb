#include "LogRecord.h"

namespace storage {
static thread_local boost::crc_32_type crc32;

LogPageDivid::LogPageDivid(uint64_t logId, Byte *buf, uint32_t bufLen,
                           PageID parentID,
                           MVector<RawRecord *>::Type &vctLastRec,
                           MVector<PageID>::Type &vctPageID, IndexPage *page)
    : LogBase(logId, buf, bufLen) {
  assert(vctLastRec.size() == vctPageID.size());
  /**
   * For divid page log, it will be below struct:
   * 4 bytes, data length for this log
   * 8 bytes, log id
   * 8 bytes, create date time for this log
   * 4 bytes, parent page id
   * 4 bytes, the number of splited pages, include origin page
   * (4 bytes + record data length) * n, page id + record data * records number
   * Save split page's origin data, if ogSaveSplitPage = true
   * 4 bytes, crc32 code
   */
  uint32_t dLen = 4 + 8 + 8 + 4 + 4 + 4 * vctPageID.size();
  for (RawRecord *rec : vctLastRec) {
    dLen += rec->GetTotalLength();
  }

  if (bufLen < dLen) {
    _bSingle = true;
    _buf = CachePool::Apply(dLen, _bufLen);
    buf = _buf;
  }

  *(uint32_t *)buf = dLen;
  buf += sizeof(uint32_t);
  *(uint64_t *)buf = logId;
  buf += sizeof(uint64_t);
  *(uint64_t *)buf = _createDt;
  buf += sizeof(uint64_t);
  *(uint32_t *)buf = parentID;
  buf += sizeof(uint32_t);
  *(uint32_t *)buf = (uint32_t)vctPageID.size();
  buf += sizeof(uint32_t);

  for (int i = 0; i < vctPageID.size(); i++) {
    *(uint32_t *)buf = vctPageID[i];
    buf += sizeof(uint32_t);
    RawRecord *rec = vctLastRec[i];
    memcpy(buf, rec->GetBysValue(), rec->GetTotalLength());
    buf += rec->GetTotalLength();
  }

  crc32.reset();
  crc32.process_bytes(_buf, dLen - sizeof(uint32_t));
  *(uint32_t *)buf = crc32.checksum();
}

bool LogPageDivid::ReadData(uint64_t &logId, PageID &parentID,
                            MVector<RawRecord *>::Type &vctLastRec,
                            MVector<PageID>::Type &vctPageID, IndexPage *page) {
  assert(_buf != nullptr && _bufLen > 0);
  Byte *buf = _buf;
  uint32_t dLen = *(uint32_t *)buf;
  assert(dLen < _bufLen);
  buf += sizeof(uint32_t);

  crc32.reset();
  crc32.process_bytes(_buf, dLen - sizeof(uint32_t));
  uint32_t code = *(uint32_t *)(_buf + dLen - sizeof(uint32_t));
  if (code != crc32.checksum()) {
    return false;
  }

  logId = *(uint64_t *)buf;
  
}
} // namespace storage