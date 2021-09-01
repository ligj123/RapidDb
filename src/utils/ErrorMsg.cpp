#include "ErrorMsg.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

namespace storage {

unordered_map<int, string> ErrorMsg::_mapErrorMsg = ErrorMsg::LoadErrorMsg();

unordered_map<int, string> ErrorMsg::LoadErrorMsg() {
  ifstream ifs("ErrorMsg.txt");
  string line;
  std::regex rgx("^\\d+(\t| {2,})");
  unordered_map<int, string> map;

  while (getline(ifs, line)) {
    if (line.size() < 4)
      continue;
    std::smatch sm;
    if (!std::regex_search(line, sm, rgx))
      continue;

    int id = std::atoi(sm[0].str().c_str());
    string msg = sm.suffix().str();
    map.insert({id, msg});
  }

  return map;
}
} // namespace storage
