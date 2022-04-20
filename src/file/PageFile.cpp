#include "PageFile.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
thread_local OvfBuffer PageFile::_ovfBuff;

PageFile::PageFile(const string &path, bool overflowFile) {
  _bOverflowFile = overflowFile;
  _path = path;
  _file.open(path, ios::in | ios::out | ios::binary);
  if (!_file.is_open()) {
    _file.open(path, ios::out);
    if (!_file.is_open())
      throw ErrorMsg(FILE_OPEN_FAILED, {path});
    _file.close();
    _file.open(path, ios::in | ios::out | ios::binary);
  }

  if (_bOverflowFile) {
    uint64_t len = Length();
    len += Configure::GetDiskClusterSize() - 1;
    len = len - len % Configure::GetDiskClusterSize();
    _overFileLength.store(len);
  }
}

uint32_t PageFile::ReadPage(uint64_t fileOffset, char *bys, uint32_t length) {
  // assert(fileOffset % Configure::GetDiskClusterSize() == 0);
  assert(Length() > fileOffset);
  uint32_t len = 0;
  {
    lock_guard<SpinMutex> lock(_spinMutex);
    _file.seekp(fileOffset, ios::beg);
    _file.read(bys, length);
    uint32_t len = (uint32_t)_file.gcount();
  }
  LOG_DEBUG << "Read a page, offset=" << fileOffset << "  length=" << length
            << "  name=" << _path;
  return len;
}

void PageFile::WritePage(uint64_t fileOffset, char *bys, uint32_t length) {
  // assert(fileOffset % Configure::GetDiskClusterSize() == 0);
  {
    lock_guard<SpinMutex> lock(_spinMutex);
    _file.seekp(fileOffset, ios::beg);
    _file.write(bys, length);
  }

  LOG_DEBUG << "Write a page, offset=" << fileOffset << "  length=" << length
            << "  name=" << _path;
}

} // namespace storage
