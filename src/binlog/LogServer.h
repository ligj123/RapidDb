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
  static SpinMutex _spinMutex;
  static queue<Byte *> _queueLog;
  // Used to generate serial number for every log record. It will ensure the
  // strict sequence when recover from machine crash or restore duplicates on
  // other machines.
  static uint64_t _genLogId;

protected:
  // To generate log id
  // The log id is 64 bits uint64_t. The highest 16 bits is reserved for
  // distributed version. The following 8 bits is to save this node restart how
  // many times, add one every time and recycle from 0 to 255. The last 40 bit
  // is self increasing integer from 0 to 0x000000ffffffffff.
  static atomic_uint64_t _atomicLogId;
};
} // namespace storage