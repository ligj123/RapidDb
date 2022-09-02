#include "LogServer.h"
#include <fstream>

namespace storage {
uint64_t LogServer::_currLogId = UINT64_MAX;
LogServer *LogServer::_inst = nullptr;

void LogServer::StartServer() {
  assert(_inst = nullptr);
  if (_currLogId == UINT64_MAX) {
    fstream file("pid", ios::in | ios::out | ios::binary);
    Byte by;
    file.read((char *)&by, 1);
    _currLogId =
        ((uint64_t)Configure::GetNodeId() << 48) + ((uint64_t)by << 40);
    file.seekp(0);
    by++;
    file.write((char *)&by, 1);
    file.close();
  }

  _inst = new LogServer();
}

void LogServer::StopServer() { _inst->_bStop = true; }

void LogServer::LogWriteThread() {
  fstream logFile(Configure::GetLogPath() + StrSecTime() + ".bin",
                  ios::in | ios::out | ios::binary);
  uint32_t fLen = UI64_LEN;
  // Write the oldest active transaction's create time. This date will be used
  // to split recover log files.
  DT_MilliSec dt = 0; // Get time
  logFile.write((const char *)&dt, UI64_LEN);

  const uint32_t blfSz = Configure::GetBinLogFileSize();
  {
    std::unique_lock<SpinMutex> lock(_inst->_mutexOut);
    _inst->_cv.wait_for(lock, 1ms, []() -> bool {
      return _inst->_tail != nullptr && _inst->_bStop;
    });
  }
  _inst->_fileLog = _inst->_head;
  logFile.write((char *)_inst->_fileLog->GetBuf(),
                _inst->_fileLog->GetLength());
  fLen += _inst->_fileLog->GetLength();

  while (!_inst->_bStop && _inst->_fileLog->_next == nullptr) {
    std::unique_lock<SpinMutex> lock(_inst->_mutexOut);
    _inst->_cv.wait_for(lock, 1ms, []() -> bool {
      return _inst->_fileLog->_next != nullptr && _inst->_bStop;
    });

    while (_inst->_fileLog->_next != nullptr && fLen < blfSz) {
      _inst->_fileLog = _inst->_fileLog->_next;
      logFile.write((char *)_inst->_fileLog->GetBuf(),
                    _inst->_fileLog->GetLength());
      fLen += _inst->_fileLog->GetLength();
    }

    if (fLen >= blfSz) {
      logFile.close();
      logFile.open(Configure::GetLogPath() + StrSecTime() + ".bin",
                   ios::in | ios::out | ios::binary);
      fLen = 0;
      DT_MilliSec dt = UI64_LEN; // Get time
      logFile.write((const char *)&dt, UI64_LEN);
    }

    while (_inst->_head != _inst->_fileLog) {
      LogBase *tmp = _inst->_head;
      _inst->_head = _inst->_head->_next;
      delete tmp;
    }
  }

  logFile.close();
  while (_inst->_head != nullptr) {
    LogBase *tmp = _inst->_head;
    _inst->_head = _inst->_head->_next;
    delete tmp;
  }
}
} // namespace storage