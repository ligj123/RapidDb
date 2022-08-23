#include "LogRecord.h"

namespace storage {
LogPageDivid::LogPageDivid(uint64_t logId, Byte *buf, uint32_t bufLen,
                           PageID parentID,
                           MVector<RawRecord *>::Type &vctLastRec,
                           MVector<PageID>::Type &vctPageID, IndexPage *page)
    : LogBase(logId, buf, bufLen) {}

LogPageDivid::LogPageDivid(Byte *buf, uint64_t len) : LogBase(buf, len) {}
} // namespace storage