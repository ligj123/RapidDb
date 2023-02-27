#pragma once
#include "../cache/Mallocator.h"
#include <exception>
#include <mutex>
#include <unordered_map>

namespace storage {
using namespace std;
class ErrorMsg;
static thread_local ErrorMsg *_threadErrorMsg = nullptr;

class ErrorMsg : public exception {
public:
  ErrorMsg() {}
  ErrorMsg(int id, MVector<string>::Type &&paras = {}) {
    SetMsg(id, move(paras));
  }

  ErrorMsg(ErrorMsg &&msg) noexcept {
    _errId = msg._errId;
    _errMsg = move(msg._errMsg);
  }

  ErrorMsg &operator=(ErrorMsg &&msg) noexcept {
    _errId = msg._errId;
    _errMsg = move(msg._errMsg);
    return *this;
  }

  inline void SetMsg(int errId, MVector<string>::Type &&paras = {}) {
    _errId = errId;
    auto iter = _mapErrorMsg.find(errId);
    if (iter == _mapErrorMsg.end()) {
      _errMsg = string("Failed to find the error id, id=" + to_string(errId));
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

  const char *what() const noexcept { return _errMsg.c_str(); }
  int getErrId() { return _errId; }
  int GetSize() { return UI32_LEN * 2 + (int)_errMsg.size(); }
  void SaveMsg(Byte *buf);
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
