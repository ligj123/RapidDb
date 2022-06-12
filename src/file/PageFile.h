#pragma once
#include "../cache/Mallocator.h"
#include "../config/ErrorID.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include "../utils/SpinMutex.h"
#include <atomic>
#include <fstream>
#include <iostream>

namespace storage {
using namespace std;

class PageFile {
public:
  static thread_local char _tmpBuff[1024 * 1024];

  PageFile(const string &path);

  ~PageFile() {
    if (_file.is_open())
      _file.close();
  }

  uint32_t ReadPage(uint64_t fileOffset, char *bys, uint32_t length);
  void WritePage(uint64_t fileOffset, char *bys, uint32_t length);

  uint64_t Length() {
    _file.seekp(0, ios::end);
    return _file.tellp();
  }

  void close() { _file.close(); }

protected:
  string _path;
  fstream _file;
};
} // namespace storage
