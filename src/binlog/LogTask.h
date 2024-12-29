#pragma once
#include "../serv/Transaction.h"
#include "../utils/RapidQueue.h"
#include "../utils/ThreadPool.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace storage {
class LogTask : public ThreadTask {
public:
  TaskStatus Run() override;

protected:
  RapidQueue<Transaction> _queueTran;
  MString _logPath;
  MString _logFileName;
  fstream _logFile;
  uint32_t _fileLength;
};
} // namespace storage