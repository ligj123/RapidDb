#include "LogServer.h"

namespace storage {
// Generate a log row when insert a record, then push the log row into a queue
// and wait to write disk or send to other machine.
// Every log row include:8 bytes log id + 8 bytes transaction id +
//
void LogServer::PushRecordInsert(uint64_t tranId, const LeafRecord &rec) {}

void LogServer::PushRecordUpdate(uint64_t tranId, const LeafRecord &rec) {}

void LogServer::PushRecordDelete(uint64_t tranId, const LeafRecord &rec) {}
} // namespace storage