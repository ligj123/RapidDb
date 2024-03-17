#pragma once

#include "../cache/Mallocator.h"
#include "ErrorID.h"
#include "ErrorMsg.h"

#ifdef LINUX_OS
#include <errno.h>
#include <fcntl.h>
#include <libaio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef int FILE_HANDLE;
#endif

namespace storage {
using namespace std;

class FileHandle {
public:
  static FileHandle *OpenFile(const MString &indexPath) {
    FileHandle *fh = new FileHandle(indexPath);
    if (!fh->_bValid) {
      delete fh;
      return nullptr;
    }

    return fh;
  }

  FILE_HANDLE FileDescriptor() { return _fd; }
  bool Close() {
#ifdef LINUX_OS
    if (close(_fd) != 0) {
      _threadErrorMsg.reset(new ErrorMsg(FILE_CLOSE_FAILED, {_indexPath}));
      return false;
    }
#endif
  }

protected:
  FileHandle(const MString &indexPath) : _indexPath(indexPath) {
#ifdef LINUX_OS
    _fd = open(_indexPath.c_str(), O_CREAT | O_RDWR | O_DIRECT, 0644);
    if (_fd < 0) {
      _threadErrorMsg.reset(new ErrorMsg(FILE_OPEN_FAILED, {_indexPath}));
      _bValid = false;
    }
#endif

    _bValid = true;
  }

protected:
  MString _indexPath; // The index tree's file path
  bool _bValid;       // If this file open right
  FILE_HANDLE _fd;    // file descriptor
};

} // namespace storage