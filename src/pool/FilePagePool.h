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

namespace storage {
using namespace std;

class FilePagePool {
public:
  static void Start();
  static void Stop();

public:
  void AddReadPage(uint16_t tid, CachePage *page) {
    _readQueue.Push(tid, page, true);
  }
  void AddWritePage(uint16_t tid, CachePage *page) {
    _writeQueue.Push(tid, page, true);
  }

protected:
  void Run();

protected:
  static FilePagePool *_pool;
  thread _thread;
  // The cache page that need to read
  FastQueue<CachePage, 1000> _readQueue;
  // The cache pages that need to write
  FastQueue<CachePage, 1000> _writeQueue;

#ifdef LINUX_OS
  io_context_t _context;
  struct iocb _iocb[100];
  struct io_event _event[100];
  struct timespec _timeout;
#endif
};
} // namespace storage