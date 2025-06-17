#include "LogTask.h"

#include "../core/BranchRecord.h"
#include "../serv/Transaction.h"
#include "../statement/Statement.h"
#include "../table/Table.h"
#include "../utils/Log.h"

#include <filesystem>

namespace storage {
namespace fs = std::filesystem;

RapidQueue<Transaction> *LogTask::_queueTran{nullptr};
LogTask *LogTask::_logTask{nullptr};

bool LogTask::InitLogTask(ThreadPool *threadPool, const MString &logPath,
                          bool bExclusive) {
  assert(_queueTran == nullptr && _logTask == nullptr);
  _queueTran = new RapidQueue<Transaction>(threadPool->GetMaxThreads(),
                                           threadPool->GetAliveThreadCount());
  _logTask = new LogTask(threadPool, logPath);
  _logTask->SetExclusiveTask(bExclusive);
  threadPool->AddTask(_logTask);
  return true;
}

LogTask::LogTask(ThreadPool *threadPool, const MString &logPath)
    : ThreadTask(threadPool), _logPath(logPath) {
  _taskName = "LogTask";
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
  SetStatus(TaskStatus::RUNNING, false);
  MList<Transaction *> lstTran;
  _queueTran->Pop(lstTran);
  for (Transaction *tran : lstTran) {
    tran->WriteLog(this);
  }

  _logStream.flush();

  for (Transaction *tran : lstTran) {
    tran->SetLogged();
  }

  if (ThreadPool::IsStoped() && lstTran.size() == 0) {
    _tryStopTime--;
    if (_tryStopTime == 0) {
      return TaskStatus::FINISHED;
    }
  }

  SetStatus(TaskStatus::INTERVAL, false);
  return TaskStatus::INTERVAL;
}

void LogTask::WriteBuff(Byte *buff, int64_t dataLen) {
  _logStream.write(reinterpret_cast<char *>(_buff), dataLen);
  _fileLength += dataLen;
}

void LogTask::RecreateLogFile() {
  assert(_fileLength >= LOG_FILE_LEN_LIMIT);

  _logStream.close();
  _logFileName = _logPath + PREFIX_LOG_NAME + ToMString(SecondTime()) + ".blog";
  _logStream = fstream(_logFileName.c_str(),
                       ios_base::binary | ios_base::out | ios_base::app);
  if (!_logStream.is_open()) {
    LOG_FATAL << "Failed to recreate redo log file, file name=" << _logFileName;
    abort();
  }

  _fileLength = 0;
}

void LogTask::Clear() {
  MList<Transaction *> lst;
  _queueTran->Pop(lst);
  _logTask->SetStatus(TaskStatus::FINISHED, true);
  CloseTask();
}

void LogTask::WriteDmlLog(Transaction *tran) {
  boost::crc_32_type crc32;

  Byte *cBuff = _buff;
  *((uint64_t *)cBuff) = tran->GetTranID();
  cBuff += UI64_LEN;
  *cBuff = (Byte)LogType::DML_OP;
  cBuff++;

  MHashMap<PhysTable *, TreeSetRecord> mapSetRec;
  auto &lstStmt = tran->GetListStatement();
  for (auto iter = lstStmt.rbegin(); iter != lstStmt.rend(); iter++) {
    assert(tran->GetTranID() == (*iter)->GetTxId());
    (*iter)->CollectLogRecords(mapSetRec);
  }

  *((uint32_t *)cBuff) = (uint32_t)mapSetRec.size();
  cBuff += UI32_LEN;

  for (auto iter = mapSetRec.begin(); iter != mapSetRec.end(); iter++) {
    PhysTable *tbl = iter->first;
    const MString &fname = tbl->GetFullName();
    if (cBuff - _buff < UI32_LEN * 2 + fname.size()) {
      crc32.process_bytes(_buff, cBuff - _buff);
      WriteBuff(_buff, cBuff - _buff);
    }

    *((uint32_t *)cBuff) = tbl->TableID();
    cBuff += UI32_LEN;
    *((uint32_t *)cBuff) = (uint32_t)fname.size();
    cBuff += UI32_LEN;
    BytesCopy(cBuff, fname.c_str(), fname.size());
    cBuff += fname.size();

    for (LeafRecord *lr : iter->second) {
      uint16_t len = lr->GetActualLength();
      if (cBuff - _buff > BUFF_SIZE - len) {
        crc32.process_bytes(_buff, cBuff - _buff);
        WriteBuff(_buff, cBuff - _buff);
        cBuff = _buff;
      }

      BytesCopy(cBuff, lr->GetBysValue(), len);
      cBuff += len;

      if (lr->HasOverflowPage()) {
        OverflowPage *ovPage = lr->GetOverflowPage();
        int ovfLen = ovPage->PageSize();
        if (cBuff - _buff > BUFF_SIZE - ovfLen - UI32_LEN) {
          crc32.process_bytes(_buff, cBuff - _buff);
          WriteBuff(_buff, cBuff - _buff);
          cBuff = _buff;
        }

        if (ovfLen + UI32_LEN > BUFF_SIZE) {
          crc32.process_bytes(reinterpret_cast<char *>(&ovfLen), UI32_LEN);
          crc32.process_bytes(reinterpret_cast<char *>(ovPage->GetBysPage()),
                              ovfLen);

          _logStream.write(reinterpret_cast<char *>(&ovfLen), UI32_LEN);
          _logStream.write(reinterpret_cast<char *>(ovPage->GetBysPage()),
                           ovfLen);
        } else {
          *((uint32_t *)cBuff) = ovfLen;
          cBuff += UI32_LEN;
          BytesCopy(cBuff, ovPage->GetBysPage(), ovPage->PageSize());
          cBuff += ovPage->PageSize();
        }
      }
    }
  }

  crc32.process_bytes(_buff, cBuff - _buff);
  WriteBuff(_buff, cBuff - _buff);
  uint32_t c = crc32.checksum();
  _logStream.write(reinterpret_cast<char *>(&c), UI32_LEN);
}

void LogTask::WriteSplitePageLog(Transaction *tran) {
  SplitPageTran *spTran = dynamic_cast<SplitPageTran *>(tran);
  assert(spTran != nullptr);

  boost::crc_32_type crc32;
  Byte *cBuff = _buff;
  *reinterpret_cast<uint64_t *>(cBuff) = tran->GetTranID();
  cBuff += UI64_LEN;
  *cBuff = static_cast<Byte>(LogType::PAGE_SPLIT);
  cBuff++;

  *reinterpret_cast<PageID *>(cBuff) = spTran->GetParentPageID();
  cBuff += UI32_LEN;
  *reinterpret_cast<PageID *>(cBuff) = spTran->GetSplitPageID();
  cBuff += UI32_LEN;

  MVector<BranchRecord *> &vctBr = spTran->GetVctNewRec();
  *reinterpret_cast<uint32_t *>(cBuff) = static_cast<uint32_t>(vctBr.size());
  cBuff += UI32_LEN;

  for (BranchRecord *br : vctBr) {
    BytesCopy(cBuff, br->GetBysValue(), br->GetTotalLength());
    cBuff += br->GetTotalLength();
  }

  crc32.process_bytes(_buff, cBuff - _buff);
  *reinterpret_cast<uint32_t *>(cBuff) = crc32.checksum();
  cBuff += UI32_LEN;
  WriteBuff(_buff, cBuff - _buff);
}

void LogTask::WriteDdlLog(Transaction *tran) {
  MList<Statement *> &lstStmt = tran->GetListStatement();
  assert(lstStmt.size() == 1);
  Statement *stmt = lstStmt.back();

  boost::crc_32_type crc32;
  Byte *cBuff = _buff;
  *reinterpret_cast<uint64_t *>(cBuff) = tran->GetTranID();
  cBuff += UI64_LEN;
  *cBuff = static_cast<Byte>(LogType::DDL_OP);
  cBuff++;

  ExprType ty = stmt->GetType();
  *cBuff = static_cast<Byte>(ty);
  cBuff++;

  switch (ty) {
  case ExprType::EXPR_CREATE_DATABASE:
  case ExprType::EXPR_CREATE_TABLE:
  case ExprType::EXPR_DROP_TABLE: {
    MList<LeafRecord *> &lstRec = stmt->GetRecords();
    assert(lstRec.size() == 1);
    BytesCopy(cBuff, lstRec.back()->GetBysValue(),
              lstRec.back()->GetActualLength());
    cBuff += lstRec.back()->GetActualLength();
    break;
  }
  case ExprType::EXPR_DROP_DATABASE: {
    MList<LeafRecord *> &lstRec = stmt->GetRecords();
    *reinterpret_cast<uint32_t *>(cBuff) = static_cast<uint32_t>(lstRec.size());
    cBuff += UI32_LEN;
    for (LeafRecord *lr : lstRec) {
      BytesCopy(cBuff, lr->GetBysValue(), lr->GetActualLength());
      cBuff += lr->GetActualLength();
    }
    break;
  }
  case ExprType::EXPR_TRUN_TABLE:
  default:
    LOG_INFO << "Unsupport operation: " << ty;
    abort();
    break;
  }

  crc32.process_bytes(_buff, cBuff - _buff);
  *reinterpret_cast<uint32_t *>(cBuff) = crc32.checksum();
  cBuff += UI32_LEN;
  WriteBuff(_buff, cBuff - _buff);
}
} // namespace storage