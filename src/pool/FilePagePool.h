#pragma once
#include "../core/CachePage.h"
#include "../utils/RapidQueue.h"
#include "../utils/SpinMutex.h"
#include <thread>

#ifdef LINUX_OS
#include <errno.h>
#include <fcntl.h>
#include <libaio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#define IOCB_SIZE 128

namespace storage {
using namespace std;

class FilePagePool {
public:
  static void Start(uint16_t lineNum);
  static void Stop();
  static void AddReadPage(uint16_t tid, CachePage *page, bool submit = true) {
    assert(page->GetPageStatus() == PageStatus::EMPTY);
    page->SetPageStatus(PageStatus::READING);
    _pool->_readRapidQueue.Push(tid, page, submit);
  }
  static void AddWritePage(uint16_t tid, CachePage *page, bool submit = true) {
    assert(page->GetPageStatus() == PageStatus::VALID);
    page->SetPageStatus(PageStatus::WRITING);
    _pool->_writeRapidQueue.Push(tid, page, submit);
  }
  static void SubmitWritePage(uint16_t tid) {
    _pool->_writeRapidQueue.Submit(tid);
  }

  static bool SyncReadPage(CachePage *page);
  static bool SyncWritePage(CachePage *page);

  FilePagePool(uint16_t lineNum);

protected:
  void Run();

protected:
  static FilePagePool *_pool;
  thread _thread;
  bool _bStop{false};
  // The cache pages that need to read
  RapidQueue<CachePage> _readRapidQueue;
  // The cache pages that need to write
  RapidQueue<CachePage> _writeRapidQueue;

  MDeque<CachePage *> _readMQueue;
  MDeque<CachePage *> _writeMQueue;

#ifdef LINUX_OS
  void InitHandle();
  void RWPage();
  io_context_t _context;
  struct iocb _iocb[IOCB_SIZE];
  struct iocb *_piocb[IOCB_SIZE];
  struct io_event _event[IOCB_SIZE];
  struct timespec _timeout;
#endif
};
} // namespace storage