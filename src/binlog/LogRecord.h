#pragma once
#include "../cache/Mallocator.h"
#include "../core/BranchRecord.h"
#include "../core/IndexPage.h"
#include "../utils/Utilitys.h"

namespace storage {
// Base class for all log type, it will be used to
class LogBase {
public:
  LogBase(uint64_t logId, Byte *buf, uint32_t bufLen)
      : _logId(logId), _createDt(MicroSecTime()), _buf(buf), _bufLen(bufLen),
        _bSingle(buf != nullptr), _next(nullptr) {
    assert((buf == nullptr && bufLen == 0) || (buf != nullptr && bufLen > 0));
  }
  LogBase(Byte *buf, uint32_t bufLen)
      : _logId(0), _createDt(0), _buf(buf), _bufLen(bufLen), _bSingle(false),
        _next(nullptr) {
    assert(buf != nullptr && bufLen > 0);
  }
  ~LogBase() {
    if (_bSingle)
      CachePool ::Release(_buf, _bufLen);
  }

  Byte *GetBuf() {
    assert(_buf != nullptr);
    return _buf;
  }

  uint32_t GetLength() {
    assert(_bufLen > 0);
    return _bufLen;
  }

  bool IsSingle() { return _bSingle; }
  void SetLogId(uint64_t logId) {
    _logId = logId;
    *(uint64_t *)&_buf[5] = logId;
  }

protected:
  uint64_t _logId;
  DT_MicroSec _createDt;
  Byte *_buf;
  uint32_t _bufLen;
  bool _bSingle;
  // The log records will generate a chain one by one, this pointer will point
  // next log record.
  LogBase *_next;

  friend class LogServer;
};

// The log for branch and leaf page split
class LogPageDivid : public LogBase {
public:
  LogPageDivid(uint64_t logId, Byte *buf, uint32_t bufLen, PageID parentID,
               MVector<BranchRecord *> &vctLastRec,
               IndexPage *page = nullptr);
  LogPageDivid(Byte *buf, uint32_t len) : LogBase(buf, len) {}
  bool ReadData(uint64_t &logId, PageID &parentID,
                MVector<BranchRecord *> &vctLastRec,
                IndexPage *page = nullptr);
};

// The log for lead record
// class LogLeafRecord : public LogBase {
// public:
//   LogLeafRecord(uint64_t logId, uint32_t indexId, uint32_t pageId,
//                 LeafRecord *lr);
//   LogLeafRecord(Byte *buf, uint32_t len) : LogBase(buf, len) {}
//   bool ReadData(uint64_t &logId, uint32_t &indexId, uint32_t &pageId,
//                 LeafRecord *&lr);
// };
} // namespace storage
