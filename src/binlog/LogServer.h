#pragma once
#include "../core/LeafRecord.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include "LogRecord.h"
#include <queue>
#include <tuple>

using namespace std;

namespace storage {
class LogServer {
public:
  static void PushRecord(LogBase *log) {
    unique_lock<SpinMutex> lock(_spinMutex);
    log->SetLogId(_atomicLogId++);
    if (_tail == nullptr) {
      _tail = log;
    } else {
      _tail->SetNext(log);
    }
  }

protected:
  static void LogWriteThread();

protected:
  static SpinMutex _spinMutex;
  // The head for record chain, it will be removed
  static LogBase *_head;
  // Newest log record will add into tail and then it change to the tail
  static LogBase *_tail;
  // The current pointer the record has write into log files.
  static LogBase *_fileLog;
  // Used in future, tuple<node id, the current pointer the record has sent, the
  // current pointer the record has verified>
  static vector<tuple<uint16_t, LogBase *, LogBase *>> _nodeLog;
  // To generate log id
  // The log id is 64 bits uint64_t. The highest 16 bits is reserved for
  // distributed version. The following 8 bits is to save this node restart how
  // many times, add one every time and recycle from 0 to 255. The last 40 bit
  // is self increasing integer from 0 to 0x000000ffffffffff.
  static uint64_t _atomicLogId;
};
} // namespace storage