#pragma once
#include "../core/LeafRecord.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include <queue>

using namespace std;

namespace storage {
class LogServer {
public:
  void PushRecordInsert(uint64_t tranId, const LeafRecord &rec);
  void PushRecordUpdate(uint64_t tranId, const LeafRecord &rec);
  void PushRecordDelete(uint64_t tranId, const LeafRecord &rec);

protected:
  SpinMutex _spinMutex;
  queue<Byte *> _queueLog;
  // Used to generate serial number for every log record. It will ensure the
  // strict sequence when recover from machine crash or restore duplicates on
  // other machines.
  uint64_t _genLogId;
};
} // namespace storage