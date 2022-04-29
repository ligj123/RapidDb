#pragma once
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include <TimerThread.h>
#include <chrono>
#include <regex>
#include <string>

namespace storage {
using namespace std;
static const char *NAME_PATTERN =
    "^[_a-zA-Z\\\\u4E00-\\\\u9FA5][_a-zA-Z0-9\\\\u4E00-\\\\u9FA5]*?$";
static regex reg(""); // NAME_PATTERN);

static void IsValidName(string name) {
  cmatch mt;
  if (!regex_match(name.c_str(), mt, reg)) {
    throw ErrorMsg(storage::TB_INVALID_FILE_VERSION, {name});
  }
}

static DT_MilliSec MSTime() { return TimerThread::GetCurrTime() / 1000; }

static std::string StrMSTime() {
  return std::to_string(TimerThread::GetCurrTime() / 1000);
}
} // namespace storage
