#pragma once
#include <exception>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace utils {
using namespace std;

class ErrorMsg : public exception {
public:
  ErrorMsg(int id, vector<string> paras = {}) {
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
          _errMsg.replace(pos, 2, paras[i]);
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
} // namespace utils
