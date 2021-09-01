﻿#pragma once
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
class OvfBuffer {
public:
  OvfBuffer() { pBuf = new char[Configure::GetMaxOverflowCache()]; }
  ~OvfBuffer() { delete[] pBuf; }

  char *GetBuf() { return pBuf; }

protected:
  char *pBuf;
};

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

  void WriteDataValue(MVector<IDataValue *>::Type vctDv, uint32_t dvStart,
                      uint64_t offset);

  void ReadDataValue(MVector<IDataValue *>::Type vctDv, uint32_t dvStart,
                     uint64_t offset, uint32_t totalLen);

  void MoveOverflowData(uint64_t fileOffsetSrc, uint64_t fileOffsetDest,
                        uint32_t length);

  uint64_t Length() {
    unique_lock<SpinMutex> lock;
    _file.seekp(0, ios::end);
    return _file.tellp();
  }

  uint64_t GetOffsetAddLength(uint32_t len) {
    assert(_bOverflowFile);
    return _overFileLength.fetch_add(len);
  }

  void close() { _file.close(); }

protected:
  /***/
  bool _bOverflowFile;
  string _path;
  fstream _file;
  SpinMutex _spinMutex;
  atomic<uint64_t> _overFileLength;
};
} // namespace storage
