#include "LogRecord.h"

namespace storage {
LogPageDivid::LogPageDivid(uint64_t logId, PageID parentID,
                           MVector<RawRecord *> vctLastRec,
                           MVector<PageID> vctPageID)
    : LogBase(logId) {}
LogPageDivid::LogPageDivid(Byte *buf, uint64_t len) {}
uint64_t LogPageDivid::SaveBuffer(Byte *buf, uint64_t bufSz) {}
void LogPageDivid::ReadBuffer(Byte *buf, uint64_t len) {}
} // namespace storage