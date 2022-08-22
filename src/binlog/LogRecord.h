#pragma once
#include "../cache/Mallocator.h"
#include "../core/RawRecord.h"
#include "../header.h"

namespace storage {
// Base class for all log type, it will be used to
class LogBase {
public:
  LogBase(uint64_t logId) : _logId(logId), _createDt(0) {}
  LogBase() : _logId(0), _createDt(0) {}

  virtual uint64_t SaveBuffer(Byte *buf, uint64_t bufSz) = 0;
  virtual void ReadBuffer(Byte *buf, uint64_t len) = 0;

protected:
  uint64_t _logId;
  DT_MicroSec _createDt;
};

// The log for branch and leaf page split
class LogPageDivid : public LogBase {

public:
  LogPageDivid(uint64_t logId, PageID parentID, MVector<RawRecord *> vctLastRec,
               MVector<PageID> vctPageID);
  LogPageDivid(Byte *buf, uint64_t len);
  uint64_t SaveBuffer(Byte *buf, uint64_t bufSz) override;
  void ReadBuffer(Byte *buf, uint64_t len) override;

protected:
  PageID _parentId;
  MVector<RawRecord *> _vctLastRec;
  MVector<PageID> _vctPageID;
};
} // namespace storage
