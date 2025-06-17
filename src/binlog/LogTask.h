#pragma once
#include "../serv/Transaction.h"
#include "../utils/RapidQueue.h"
#include "../utils/ThreadPool.h"
#include "LogType.h"

#include <boost/crc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

#define PREFIX_LOG_NAME "/redo_log_"
#define LOG_FILE_LEN_LIMIT 64000000
#define BUFF_SIZE 10000000

namespace storage {
class Transaction;
class LogTask : public ThreadTask {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  static bool InitLogTask(ThreadPool *threadPool, const MString &logPath,
                          bool bExclusive = false);

  static void CloseTask() {
    assert(_logTask->GetStatus(false) == TaskStatus::FINISHED);

    delete _queueTran;
    delete _logTask;
    _logTask = nullptr;
    _queueTran = nullptr;
  }

  static void AddTransaction(uint16_t tid, Transaction *tran) {
    assert(_queueTran != nullptr);
    _queueTran->Push(tid, tran);
  }

  // Remove all waitting tasks in _queueTran. Only for test purpose
  static void Clear();

  static LogTask *GetTask() { return _logTask; }

public:
  LogTask(ThreadPool *threadPool, const MString &logPath);
  ~LogTask() {
    _logStream.close();
    delete[] _buff;
  }
  TaskStatus Run() override;

  void WriteBuff(Byte *buff, int64_t dataLen);
  Byte *GetBuff() { return _buff; }

protected:
  void RecreateLogFile();
  void WriteDmlLog(Transaction *tran);
  void WriteSplitePageLog(Transaction *tran);
  void WriteDdlLog(Transaction *tran);

protected:
  static LogTask *_logTask;
  static RapidQueue<Transaction> *_queueTran;
  MString _logPath;
  MString _logFileName;
  fstream _logStream;
  uint64_t _fileLength{0};

  Byte *_buff;
  boost::crc_32_type _crc32;
  int32_t _tryStopTime{5};
};

} // namespace storage