#include "ErrorMsg.h"
#include "BytesFuncs.h"
#include <fstream>
#include <iostream>
#include <regex>

namespace storage {
thread_local unique_ptr<ErrorMsg> _threadErrorMsg = nullptr;
unordered_map<int, MString> ErrorMsg::_mapErrorMsg;

void ErrorMsg::LoadErrorMsg(const string &msgPath) {
  ifstream ifs(msgPath);
  string line;
  std::regex rgx("^\\d+(\t| {2,})");
  _mapErrorMsg.clear();

  while (getline(ifs, line)) {
    if (line.size() < 4)
      continue;
    std::smatch sm;
    if (!std::regex_search(line, sm, rgx))
      continue;

    int id = std::atoi(sm[0].str().c_str());
    MString msg = sm.suffix().str().c_str();
    _mapErrorMsg.insert({id, msg});
  }
}

void ErrorMsg::SaveMsg(Byte *buf) {
  *(uint32_t *)buf = _errId;
  buf += UI32_LEN;
  *(uint32_t *)buf = (uint32_t)_errMsg.size();
  buf += UI32_LEN;
  BytesCopy((void *)buf, (void *)_errMsg.data(), _errMsg.size());
}
} // namespace storage
