﻿#pragma once
#include "../cache/Mallocator.h"
#include <exception>
#include <mutex>
#include <string>
#include <unordered_map>

namespace storage {
using namespace std;

class ErrorMsg : public exception {
public:
  ErrorMsg(int id, MVector<string>::Type paras = {}) {
    _errId = id;
    auto iter = _mapErrorMsg.find(id);
    if (iter == _mapErrorMsg.end()) {
      _errMsg = "Failed to find the error id, id=" + to_string(id);
    } else {
      _errMsg = iter->second;
      for (int i = 0; i < paras.size(); i++) {
        string str = "{" + std::to_string(i + 1) + "}";
        size_t pos = _errMsg.find(str);
        if (pos != string::npos)
          _errMsg.replace(pos, 3, paras[i]);
      }
    }
  }

  const char *what() const noexcept { return _errMsg.c_str(); }
  int getErrId() { return _errId; }

protected:
  static unordered_map<int, string> LoadErrorMsg();

protected:
  static unordered_map<int, string> _mapErrorMsg;
  int _errId;
  string _errMsg;
};
} // namespace storage
