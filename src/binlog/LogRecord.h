#pragma once
#include "../cache/Mallocator.h"
#include "../core/IndexPage.h"
#include "../core/RawRecord.h"
#include "../header.h"
#include "../utils/Utilitys.h"

namespace storage {
// Base class for all log type, it will be used to
class LogBase {
public:
  LogBase(uint64_t logId, Byte *buf, uint32_t bufLen)
      : _logId(logId), _createDt(MicroSecTime()), _buf(buf), _bufLen(bufLen),
        _bSingle(buf != nullptr) {
    assert((buf == nullptr && bufLen == 0) || (buf != nullptr && bufLen > 0));
  }
  LogBase(Byte *buf, uint32_t bufLen)
      : _logId(0), _createDt(0), _buf(buf), _bufLen(bufLen), _bSingle(false) {
    assert(buf != nullptr && bufLen > 0);
  }
  ~LogBase() {
    if (_bSingle)
      CachePool ::Release(_buf, _bufLen);
  }

  Byte *GetBuf(uint32_t &bufLen) {
    bufLen = _bufLen;
    return _buf;
  }

protected:
  uint64_t _logId;
  DT_MicroSec _createDt;
  Byte *_buf;
  uint32_t _bufLen;
  bool _bSingle;
};

// The log for branch and leaf page split
class LogPageDivid : public LogBase {
public:
  LogPageDivid(uint64_t logId, Byte *buf, uint32_t bufLen, PageID parentID,
               MVector<RawRecord *>::Type &vctLastRec,
               MVector<PageID>::Type &vctPageID, IndexPage *page = nullptr);
  LogPageDivid(Byte *buf, uint64_t len);
};
} // namespace storage
