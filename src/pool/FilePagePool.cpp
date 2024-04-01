#include "FilePagePool.h"
#include "../core/IndexTree.h"
#include "../utils/Log.h"
#include <unistd.h>

namespace storage {
FilePagePool *FilePagePool::_pool{nullptr};

void FilePagePool::Start(uint16_t tNum) {
  assert(_pool == nullptr);
  _pool = new FilePagePool(tNum);
}

void FilePagePool::Stop() {
  assert(_pool != nullptr && !_pool->_bStop);
  _pool->_bStop = true;
  _pool->_thread.join();
}

FilePagePool::FilePagePool(uint16_t tNum)
    : _readFastQueue(tNum), _writeFastQueue(tNum) {
  _thread = thread([this]() { Run(); });
}

void FilePagePool::Run() {
  InitHandle();
  while (!_bStop) {
    while (_readFastQueue.RoughEmpty() && _writeFastQueue.RoughEmpty() &&
           !_bStop) {
      std::this_thread::yield();
    }

    _readFastQueue.Pop(_readMQueue);
    _writeFastQueue.Pop(_writeMQueue);
#ifdef LINUX_OS
    RWPage();
#else
    abort();
#endif
  }
}

bool FilePagePool::SyncReadPage(CachePage *page) {
  const Byte *bys = page->GetBysPage();
  int64_t psz = page->PageSize();
  int64_t offset = page->FileOffset();
  FILE_HANDLE fh = page->GetIndexTree()->Get();

#ifdef LINUX_OS
  int64_t rt = lseek(fh, offset, SEEK_SET);
  if (rt < 0) {
    LOG_ERROR << "Failed to seek file, name="
              << page->GetIndexTree()->GetFileName() << "  ret=" << rt;
    abort();
    return false;
  }
  rt = read(fh, (void *)bys, psz);
  if (rt != psz) {
    LOG_ERROR << "Failed to read file, name="
              << page->GetIndexTree()->GetFileName() << "  ret=" << rt;
    abort();
    return false;
  }
#else
  abort();
#endif
  return true;
}

bool FilePagePool::SyncWritePage(CachePage *page) {
  const Byte *bys = page->GetBysPage();
  int64_t psz = page->PageSize();
  int64_t offset = page->FileOffset();
  FILE_HANDLE fh = page->GetIndexTree()->GetFileHandle();
#ifdef LINUX_OS
  int64_t rt = lseek(fh, offset, SEEK_SET);
  if (rt < 0) {
    LOG_ERROR << "Failed to seek file, name="
              << page->GetIndexTree()->GetFileName() << "  ret=" << rt;
    abort();
    return false;
  }
  rt = write(fh, (void *)bys, psz);
  if (rt != psz) {
    LOG_ERROR << "Failed to write file, name="
              << page->GetIndexTree()->GetFileName() << "  ret=" << rt;
    abort();
    return false;
  }
#else
  abort();
#endif
  return true;
}

#ifdef LINUX_OS
void FilePagePool::InitHandle() {
  memset(_iocb, 0, sizeof(iocb) * IOCB_SIZE);
  for (int i = 0; i < IOCB_SIZE; i++) {
    _piocb[i] = &_iocb[i];
  }

  memset(&_context, 0, sizeof(io_context_t));
  if (0 != io_setup(IOCB_SIZE, &_context)) {
    LOG_FATAL << "io_setup error: " << errno;
    abort();
  }

  _timeout.tv_sec = 0;
  _timeout.tv_nsec = 100000;
}

void FilePagePool::RWPage() {
  while (true) {
    size_t count = 0;
    while (count < IOCB_SIZE && _readMQueue.size() > 0) {
      CachePage *page = _readMQueue.front();
      _readMQueue.pop_front();
      void *bys = page->GetBysPage();
      uint64_t psz = page->PageSize();
      int64_t offset = page->FileOffset();
      FILE_HANDLE fh = page->GetIndexTree()->GetFileHandle();

      io_prep_pread(_piocb[count], fh, bys, psz, offset);
      _piocb[count]->data = page;
      count++;
    }

    while (count < IOCB_SIZE && _writeMQueue.size() > 0) {
      CachePage *page = _writeMQueue.front();
      _writeMQueue.pop_front();
      void *bys = page->GetBysPage();
      uint64_t psz = page->PageSize();
      int64_t offset = page->FileOffset();
      FILE_HANDLE fh = page->GetIndexTree()->GetFileHandle();

      io_prep_pwrite(_piocb[count], fh, bys, psz, offset);
      _piocb[count]->data = page;
      count++;
    }

    int ret = io_submit(_context, count, _piocb);
    if (ret != count) {
      LOG_FATAL << "io_submit error: " << ret;
      abort();
    }

    while (count > 0) {
      ret = io_getevents(_context, 1, count, _event, &_timeout);
      if (ret < 0) {
        LOG_FATAL << "io_getevents error:" << ret;
        abort();
      }

      for (int i = 0; i < ret; i++) {
        struct iocb *cb = (struct iocb *)_event[i].obj;
        CachePage *page = static_cast<CachePage *>(cb->data);
        if (page->GetPageStatus() == PageStatus::READING) {
          page->AfterRead();
        } else {
          page->AfterWrite();
        }
      }

      count -= ret;
    }

    if (_readMQueue.size() == 0 && _writeMQueue.size() == 0) {
      break;
    }
  }
}
#endif
} // namespace storage