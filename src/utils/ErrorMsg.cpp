#include "ErrorMsg.h"
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

namespace utils {

  unordered_map<int, string> ErrorMsg::_mapErrorMsg;
  once_flag ErrorMsg::_flag;

  void ErrorMsg::LoadErrorMsg()
  {
    ifstream ifs("ErrorMsg.txt");
    string line;
    while (getline(ifs, line))
    {
      if (line.size() < 4) continue;
      size_t pos = line.find('\t');
      if (pos == string::npos) continue;

      int id = std::atoi(line.c_str());
      string msg = line.substr(pos + 1);
      _mapErrorMsg.insert({ id, msg });
    }
  }
}