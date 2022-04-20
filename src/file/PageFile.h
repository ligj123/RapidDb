#pragma once
#include "../config/ErrorID.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorMsg.h"
#include "../utils/SpinMutex.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include <string>

namespace storage {
using namespace std;

class PageFile {
public:
  static thread_local OvfBuffer _ovfBuff;

  PageFile(const string &path, bool overflowFile = false);

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
  SpinMutex _spinMutex;
};
} // namespace storage
