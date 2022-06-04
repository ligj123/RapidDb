#pragma once
#include "../config/ErrorID.h"
#include "../header.h"
#include "../utils/ErrorMsg.h"
#include "TimerThread.h"
#include <chrono>
#include <regex>

namespace storage {
using namespace std;
// static const char *NAME_PATTERN =
//     "^[_a-zA-Z\\\\u4E00-\\\\u9FA5][_a-zA-Z0-9\\\\u4E00-\\\\u9FA5]*?$";
// static regex reg(""); // NAME_PATTERN);
//
static void IsValidName(MString &name) {
  // cmatch mt;
  // if (!regex_match(name.c_str(), mt, reg)) {
  //   throw ErrorMsg(storage::TB_INVALID_FILE_VERSION, {name});
  // }
}

static inline DT_MilliSec MilliSecTime() {
  DT_MilliSec dt = TimerThread::GetCurrTime() / 1000;
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return dt;
}

static inline DT_MicroSec MicroSecTime() {
  DT_MilliSec dt = TimerThread::GetCurrTime();
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::microseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return dt;
}

static inline DT_Second SecondTime() {
  DT_MilliSec dt = TimerThread::GetCurrTime() / 1000000;
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::seconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return dt;
}

static inline MString StrMSTime() {
  DT_MilliSec dt = TimerThread::GetCurrTime() / 1000;
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return ToMString(dt);
}
} // namespace storage
