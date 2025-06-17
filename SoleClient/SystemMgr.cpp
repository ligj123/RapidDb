#include "SystemMgr.h"

#include "../src/binlog/LogTask.h"
#include "../src/manager/DatabaseManager.h"
#include "../src/manager/TableManager.h"
#include "../src/pool/CachePagePool.h"
#include "../src/pool/FilePagePool.h"
#include "../src/serv/SessionPool.h"
#include "../src/sysTable/SysTable.h"
#include "../src/utils/ThreadPool.h"

namespace storage {
void SystemMgr::InitSystem() {
  ThreadPool *tpool = ThreadPool::CreateMainPool("Sole", 1, 4);
  FilePagePool::Start(4);
  SessionPool::InitPool(1, 1, 0, 1, tpool, true);
  LogTask::InitLogTask(tpool, "./binlog/", true);

  StmtResult stmtRes;
  uint32_t sid = SessionPool::CreateSession(0, &stmtRes);
  assert(sid == 0);
  uint16_t tid = ThreadPool::SetThreadId(0);
  bool b = SysTable::InitSystemTable();
  assert(b);
  ThreadPool::SetThreadId(tid);
}

void SystemMgr::CloseSystem() {
  SessionPool::ClosePool();
  ThreadPool::CloseMainPool(true);
  FilePagePool::Stop();
  TableManager::ClearTable();
  DatabaseManager::ClearDB();
  SessionPool::ClearPool();
  CachePagePool::ClearPool();
  // LogTask::Clear();
  _threadErrorMsg.reset();
  ErrorMsg::ClearErrorMsg();

  if (CachePool::GetMemoryUsed() > 0) {
    LOG_INFO << "Memory leaked: " << CachePool::GetMemoryUsed();
  }
}
} // namespace storage