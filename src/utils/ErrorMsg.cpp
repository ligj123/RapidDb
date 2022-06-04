#include "ErrorMsg.h"
#include <fstream>
#include <iostream>
#include <regex>

namespace storage {

unordered_map<int, MString> ErrorMsg::_mapErrorMsg = ErrorMsg::LoadErrorMsg();

unordered_map<int, MString> ErrorMsg::LoadErrorMsg() {
  ifstream ifs("ErrorMsg.txt");
  string line;
  std::regex rgx("^\\d+(\t| {2,})");
  unordered_map<int, MString> map;

  while (getline(ifs, line)) {
    if (line.size() < 4)
      continue;
    std::smatch sm;
    if (!std::regex_search(line, sm, rgx))
      continue;

    int id = std::atoi(sm[0].str().c_str());
    MString msg = sm.suffix().str().c_str();
    map.insert({id, msg});
  }

  return map;
}
} // namespace storage
