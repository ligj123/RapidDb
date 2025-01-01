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
  LogTask(ThreadPool *threadPool, const MString &logPath);

  TaskStatus Run() override;

  void WriteBuff(int64_t dataLen, bool bTranStart);
  Byte *GetBuff() { return _buff; }

protected:
  RapidQueue<Transaction> _queueTran;
  MString _logPath;
  MString _logFileName;
  fstream _logStream;
  uint64_t _fileLength{0};

  Byte *_buff;
  boost::crc_32_type _crc32;
  int32_t _tryStopTime{5};
};
} // namespace storage