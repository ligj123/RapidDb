#include "LogTask.h"

#include "../serv/Transaction.h"
#include "../utils/Log.h"

#include <filesystem>

namespace storage {
namespace fs = std::filesystem;

RapidQueue<Transaction> *LogTask::_queueTran{nullptr};
LogTask *LogTask::_logTask{nullptr};

bool LogTask::InitLogTask(ThreadPool *threadPool, const MString &logPath) {
  assert(_queueTran == nullptr && _logTask == nullptr);
  _queueTran = new RapidQueue<Transaction>(threadPool->GetMaxThreads(),
                                           threadPool->GetAliveThreadCount());
  _logTask = new LogTask(threadPool, logPath);
  threadPool->AddTask(_logTask);
  return true;
}

LogTask::LogTask(ThreadPool *threadPool, const MString &logPath)
    : ThreadTask(threadPool), _logPath(logPath) {
  fs::path path(logPath.c_str());
  if (!fs::exists(path)) {
    fs::create_directories(path);
  }

  _logFileName = _logPath + PREFIX_LOG_NAME + ToMString(SecondTime()) + ".log";
  _logStream = fstream(_logFileName.c_str(),
                       ios_base::binary | ios_base::out | ios_base::app);
  _buff = new Byte[BUFF_SIZE];
  if (!_logStream.is_open()) {
    LOG_FATAL << "Failed to open log file " << _logFileName;
    abort();
  }

  if (_queueTran != nullptr) {
    delete _queueTran;
  }

  _queueTran = new RapidQueue<Transaction>(threadPool->GetMaxThreads());
}

TaskStatus LogTask::Run() {
  MList<Transaction *> lstTran;
  _queueTran->Pop(lstTran);
  for (Transaction *tran : lstTran) {
    tran->WriteLog(this);
  }

  _logStream.flush();

  for (Transaction *tran : lstTran) {
    tran->SetLogged();
  }

  if (_threadPool->IsStoped() && lstTran.size() == 0) {
    _tryStopTime--;
    if (_tryStopTime == 0) {
      return TaskStatus::FINISHED;
    }
  }

  return TaskStatus::RUNNING;
}

void LogTask::WriteBuff(int64_t dataLen, bool bTranStart) {
  if (bTranStart && _fileLength + dataLen > LOG_FILE_LEN_LIMIT) {
    _logStream.close();
    _logFileName =
        _logPath + PREFIX_LOG_NAME + ToMString(SecondTime()) + ".log";
    _logStream = fstream(_logFileName.c_str(),
                         ios_base::binary | ios_base::out | ios_base::app);
    if (!_logStream.is_open()) {
      abort();
    }

    _fileLength = 0;
  }

  _logStream.write((char *)_buff, dataLen);
  _fileLength += dataLen;
}
} // namespace storage