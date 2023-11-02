#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/IDataValue.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/SpinMutex.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace storage {
using namespace std;

class PageFile {
public:
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
  bool IsValid() { return _bValid; }

protected:
  filesystem::path _path;
  fstream _file;
  bool _bValid;
};
} // namespace storage
