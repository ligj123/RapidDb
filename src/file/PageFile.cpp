#include "PageFile.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
thread_local char PageFile::_tmpBuff[1024 * 1024];

PageFile::PageFile(const string &path) {
  _path = path;
  _file.open(path.c_str(), ios::in | ios::out | ios::binary);
  if (!_file.is_open()) {
    _file.open(path.c_str(), ios::out);
    if (!_file.is_open())
      throw ErrorMsg(FILE_OPEN_FAILED, {path});
    _file.close();
    _file.open(path.c_str(), ios::in | ios::out | ios::binary);
  }
}

uint32_t PageFile::ReadPage(uint64_t fileOffset, char *bys, uint32_t length) {
  assert(fileOffset % Configure::GetDiskClusterSize() == 0);
  assert(Length() > fileOffset);
  uint32_t len = 0;

  _file.seekp(fileOffset, ios::beg);
  _file.read(bys, length);
  len = (uint32_t)_file.gcount();

  assert(len == length);
  LOG_DEBUG << "Read a page, offset=" << fileOffset << "  length=" << length
            << "  name=" << _path;
  return len;
}

void PageFile::WritePage(uint64_t fileOffset, char *bys, uint32_t length) {
  assert(fileOffset % Configure::GetDiskClusterSize() == 0);

  _file.seekp(fileOffset, ios::beg);
  _file.write(bys, length);

  LOG_DEBUG << "Write a page, offset=" << fileOffset << "  length=" << length
            << "  name=" << _path;
}

} // namespace storage
