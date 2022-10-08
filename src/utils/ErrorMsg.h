#pragma once
#include "../cache/Mallocator.h"
#include "../utils/BytesMicro.h"
#include <exception>
#include <mutex>
#include <unordered_map>

namespace storage {
using namespace std;

class ErrorMsg : public exception {
public:
  ErrorMsg() {}
  ErrorMsg(int id, MVector<string>::Type paras = {}) {
    _errId = id;
    auto iter = _mapErrorMsg.find(id);
    if (iter == _mapErrorMsg.end()) {
      _errMsg = string("Failed to find the error id, id=" + to_string(id));
    } else {
      _errMsg = iter->second;
      for (int i = 0; i < paras.size(); i++) {
        string str = "{" + to_string(i + 1) + "}";
        size_t pos = _errMsg.find(str);
        if (pos != string::npos)
          _errMsg.replace(pos, 3, paras[i]);
      }
    }
  }

  ErrorMsg(const ErrorMsg &msg) {
    _errId = msg._errId;
    _errMsg = msg._errMsg;
  }

  ErrorMsg &operator=(const ErrorMsg &msg) {
    _errId = msg._errId;
    _errMsg = msg._errMsg;
    return *this;
  }

  const char *what() const noexcept { return _errMsg.c_str(); }
  int getErrId() { return _errId; }
  int GetSize() { return UI32_LEN * 2 + _errMsg.size(); }
  void SaveMsg(Byte *buf) {
    *(uint32_t *)buf = _errId;
    buf += UI32_LEN;
    *(uint32_t *)buf = (uint32_t)_errMsg.size();
    buf += UI32_LEN;
    BytesCopy(buf, _errMsg.data(), _errMsg.size());
  }
  void ReadMsg(Byte *buf) {
    _errId = *(uint32_t *)buf;
    buf += UI32_LEN;
    uint32_t len = *(uint32_t *)buf;
    buf += UI32_LEN;
    _errMsg = string((char *)buf, len);
  }

protected:
  static unordered_map<int, string> LoadErrorMsg();
  static unordered_map<int, string> _mapErrorMsg;

protected:
  int _errId = 0;
  string _errMsg;
};
} // namespace storage
