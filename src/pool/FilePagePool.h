#pragma once
#include "../core/CachePage.h"
#include "../utils/FastQueue.h"
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
  static void Start(uint16_t tNum);
  static void Stop();
  static void AddReadPage(uint16_t tid, CachePage *page) {
    page->SetPageStatus(PageStatus::READING);
    _pool->_readFastQueue.Push(tid, page, true);
  }
  static void AddWritePage(uint16_t tid, CachePage *page) {
    page->SetPageStatus(PageStatus::WRITING);
    _pool->_writeFastQueue.Push(tid, page, true);
  }

  static bool SyncReadPage(CachePage *page);
  static bool SyncWritePage(CachePage *page);

  FilePagePool(uint16_t tNum);

protected:
  void Run();

protected:
  static FilePagePool *_pool;
  thread _thread;
  bool _bStop{false};
  // The cache page that need to read
  FastQueue<CachePage, 1000> _readFastQueue;
  // The cache pages that need to write
  FastQueue<CachePage, 1000> _writeFastQueue;

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