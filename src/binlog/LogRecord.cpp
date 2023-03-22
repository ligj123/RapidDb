#include "LogRecord.h"
#include "LogType.h"
#include <boost/crc.hpp>

namespace storage {
static thread_local boost::crc_32_type crc32;

LogPageDivid::LogPageDivid(uint64_t logId, Byte *buf, uint32_t bufLen,
                           PageID parentID,
                           MVector<BranchRecord *> &vctLastRec,
                           IndexPage *page)
    : LogBase(logId, buf, bufLen) {
  assert(vctLastRec.size() > 0);
  /**
   * For divid page log, it will be below struct:
   * 1 byte, LogType
   * 4 bytes, data length for this log
   * 8 bytes, log id
   * 8 bytes, create date time for this log
   * 4 bytes, parent page id
   * 4 bytes, the number of splited pages, include origin page
   * record data length * n, n is the records number
   * Save split page's origin data, if ogSaveSplitPage = true
   * 4 bytes, crc32 code
   */
  uint32_t dLen = 4 + 8 + 8 + 4 + 4 + (uint32_t)vctLastRec.size() * 4;
  for (RawRecord *rec : vctLastRec) {
    dLen += rec->GetTotalLength();
  }

  if (Configure::IsLogSaveSplitPage()) {
    dLen += (uint32_t)Configure::GetCachePageSize();
  }

  if (bufLen < dLen) {
    _bSingle = true;
    _buf = CachePool::Apply(dLen, _bufLen);
    buf = _buf;
  }

  *buf = (Byte)LogType::PAGE_SPLIT;
  buf++;
  *(uint32_t *)buf = dLen;
  buf += sizeof(uint32_t);
  *(uint64_t *)buf = logId;
  buf += sizeof(uint64_t);
  *(uint64_t *)buf = _createDt;
  buf += sizeof(uint64_t);
  *(uint32_t *)buf = parentID;
  buf += sizeof(uint32_t);
  *(uint32_t *)buf = (uint32_t)vctLastRec.size();
  buf += sizeof(uint32_t);

  for (int i = 0; i < vctLastRec.size(); i++) {
    BranchRecord *rec = vctLastRec[i];
    BytesCopy(buf, rec->GetBysValue(), rec->GetTotalLength());
    buf += rec->GetTotalLength();
  }

  if (Configure::IsLogSaveSplitPage()) {
    BytesCopy(buf, page->GetBysPage(), Configure::GetCachePageSize());
    buf += Configure::GetCachePageSize();
  }

  crc32.reset();
  crc32.process_bytes(_buf, dLen - sizeof(uint32_t));
  *(uint32_t *)buf = crc32.checksum();
}

bool LogPageDivid::ReadData(uint64_t &logId, PageID &parentID,
                            MVector<BranchRecord *> &vctLastRec,
                            IndexPage *page) {
  assert(_buf != nullptr && _bufLen > 0);
  assert(vctLastRec.size() == 0);
  assert(*_buf == (Byte)LogType::PAGE_SPLIT);

  Byte *buf = _buf;
  buf++;
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
  buf += sizeof(uint64_t);
  // DateTime
  buf += sizeof(uint64_t);
  parentID = *(PageID *)buf;
  buf += sizeof(PageID);

  uint32_t num = *(uint32_t *)buf;
  buf += sizeof(uint32_t);

  for (uint32_t i = 0; i < num; i++) {
    BranchRecord *br = new BranchRecord(nullptr, buf);
    buf += br->GetTotalLength();
    vctLastRec.push_back(br);
  }

  if (Configure::IsLogSaveSplitPage()) {
    assert(page != nullptr);
    BytesCopy(page->GetBysPage(), buf, Configure::GetCachePageSize());
  }

  return true;
}

// LogLeafRecord::LogLeafRecord(uint64_t logId, uint32_t indexId, uint32_t
// pageId,
//                              LeafRecord *lr) {}
// bool LogLeafRecord::ReadData(uint64_t &logId, uint32_t &indexId,
//                              uint32_t &pageId, LeafRecord *&lr) {}
} // namespace storage