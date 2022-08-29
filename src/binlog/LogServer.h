#pragma once
#include "../core/LeafRecord.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include "LogRecord.h"
#include <queue>
#include <thread>
#include <tuple>

using namespace std;

namespace storage {
class LogServer {
public:
  static void StartServer();
  static void StopServer();
  static void PushRecord(LogBase *log) {
    unique_lock<SpinMutex> lock(_inst->_mutexIn);
    log->SetLogId(_inst->_currLogId++);
    if (_inst->_tail == nullptr) {
      _inst->_head = _inst->_tail = log;
    } else {
      _inst->_tail->_next = log;
    }
  }

protected:
  static void LogWriteThread();
  LogServer()
      : _bStop(false), _head(nullptr), _tail(nullptr), _fileLog(nullptr) {}

protected:
  // To generate log id
  // The log id is 64 bits uint64_t. The highest 16 bits is reserved for
  // distributed version. The following 8 bits is to save this node restart how
  // many times, add one every time and recycle from 0 to 255. The last 40 bit
  // is self increasing integer from 0 to 0x000000ffffffffff.
  static uint64_t _currLogId;
  static LogServer *_inst;
  static thread _thread;
  bool _bStop;
  SpinMutex _mutexIn;
  SpinMutex _mutexOut;
  condition_variable_any _cv;
  // The head for record chain, it will be removed
  LogBase *_head;
  // Newest log record will add into tail and then it change to the tail
  LogBase *_tail;
  // The current pointer the record has write into log files.
  LogBase *_fileLog;
  // Used in future, tuple<node id, the current pointer the record has sent, the
  // current pointer the record has verified>
  vector<tuple<uint16_t, LogBase *, LogBase *>> _nodeLog;
};
} // namespace storage