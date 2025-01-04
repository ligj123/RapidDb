#pragma once
#include "../serv/Transaction.h"
#include "../utils/RapidQueue.h"
#include "../utils/ThreadPool.h"

#include <boost/crc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

#define PREFIX_LOG_NAME "/rapid_log_"
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

  static void AddTransaction(uint16_t tid, Transaction *tran) {
    assert(_queueTran != nullptr);
    _queueTran->Push(tid, tran);
  }

public:
  LogTask(ThreadPool *threadPool, const MString &logPath);

  TaskStatus Run() override;

  void WriteBuff(int64_t dataLen, bool bTranStart);
  Byte *GetBuff() { return _buff; }

protected:
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