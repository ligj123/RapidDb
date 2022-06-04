#pragma once
#include "../cache/Mallocator.h"
#include <exception>
#include <mutex>
#include <unordered_map>

namespace storage {
using namespace std;

class ErrorMsg : public exception {
public:
  ErrorMsg() {}
  ErrorMsg(int id, MVector<MString>::Type paras = {}) {
    _errId = id;
    auto iter = _mapErrorMsg.find(id);
    if (iter == _mapErrorMsg.end()) {
      _errMsg = MString("Failed to find the error id, id=" + ToMString(id));
    } else {
      _errMsg = iter->second;
      for (int i = 0; i < paras.size(); i++) {
        MString str = "{" + ToMString(i + 1) + "}";
        size_t pos = _errMsg.find(str);
        if (pos != MString::npos)
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

protected:
  static unordered_map<int, MString> LoadErrorMsg();

protected:
  static unordered_map<int, MString> _mapErrorMsg;
  int _errId = 0;
  MString _errMsg;
};
} // namespace storage
