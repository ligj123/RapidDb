#pragma once
#include <exception>
#include <unordered_map>
#include <vector>
#include<mutex>
#include <string>

namespace utils {
  using namespace std;

  class ErrorMsg :public exception
  {
  public:
    ErrorMsg(int id, vector<string> paras = {})
    {
      std::call_once(_flag, LoadErrorMsg);
      _errId = id;
      auto iter = _mapErrorMsg.find(id);
      if (iter == _mapErrorMsg.end())
      {
        _errMsg = "Failed to find error id " + to_string(id);
      }
      else
      {
        _errMsg = iter->second;
        for (int i = 0; i < paras.size(); i++)
        {
          size_t pos = _errMsg.find("{}");
          if (pos != string::npos)
            _errMsg.replace(pos, 2, paras[i]);
        }
      }
    }
    virtual const char* what()
    {
      return _errMsg.c_str();
    }
    int getErrId() { return _errId; }
  protected:
    static void LoadErrorMsg();
  protected:
    static unordered_map<int, string> _mapErrorMsg;
    static once_flag _flag;
    int _errId;
    string _errMsg;
  };
}