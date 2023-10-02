#include "PageFile.h"
#include "../config/Configure.h"
#include "../utils/Log.h"

namespace storage {
PageFile::PageFile(const string &path) : _path(path) {
  _file.open(_path, ios::in | ios::out | ios::binary);
  if (!_file.is_open()) {
    _file.open(_path, ios::out | ios::binary);
    if (!_file.is_open()) {
      _threadErrorMsg.reset(
          new ErrorMsg(FILE_OPEN_FAILED, {_path.string().c_str()}));
      LOG_ERROR << "Failed to initialize PageFile, path= " << _path.string();
      _bValid = false;
      return;
    }

    _file.close();
    _file.open(_path, ios::in | ios::out | ios::binary);
    assert(_file.is_open());
  }

  _bValid = true;
}

uint32_t PageFile::ReadPage(uint64_t fileOffset, char *bys, uint32_t length) {
  assert(fileOffset % Configure::GetDiskClusterSize() == 0);
  assert(Length() > fileOffset);

  _file.seekp(fileOffset, ios::beg);
  _file.read(bys, length);
  uint32_t len = (uint32_t)_file.gcount();

  assert(len == length);
  LOG_DEBUG << "Read a page, offset=" << fileOffset << "  length=" << length
            << "  name=" << _path.string();
  return len;
}

void PageFile::WritePage(uint64_t fileOffset, char *bys, uint32_t length) {
  assert(fileOffset % Configure::GetDiskClusterSize() == 0);

  _file.seekp(fileOffset, ios::beg);
  _file.write(bys, length);
  LOG_DEBUG << "Write a page, offset=" << fileOffset << "  length=" << length
            << "  name=" << _path.string();
}

} // namespace storage
