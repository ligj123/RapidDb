#pragma once
#include "../header.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "TimerThread.h"
#include <chrono>
#include <iomanip>
#include <regex>
#include <sstream>

namespace storage {
using namespace std;
// static const char *NAME_PATTERN =
//     "^[_a-zA-Z\\\\u4E00-\\\\u9FA5][_a-zA-Z0-9\\\\u4E00-\\\\u9FA5]*?$";
// static regex reg(""); // NAME_PATTERN);
//
static void IsValidName(const string &name) {
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

static inline string StrMSTime() {
  DT_MilliSec dt = TimerThread::GetCurrTime() / 1000;
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return to_string(dt);
}

static inline MString MStrMSTime() {
  DT_MilliSec dt = TimerThread::GetCurrTime() / 1000;
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return ToMString(dt);
}

static inline string StrSecTime() {
  DT_Second dt = TimerThread::GetCurrTime() / 1000000;
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::seconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  return to_string(dt);
}

static inline string FormatTime() {
  int64_t dt = TimerThread::GetCurrTime();
  if (dt == 0) {
    dt = chrono::duration_cast<chrono::microseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count();
  }
  time_t sec = dt / 1000000;
  stringstream ss;
  ss << put_time(gmtime(&sec), "%Y-%m-%d %H:%M:%S") << "."
     << to_string(dt % 1000000);
  return ss.str();
}

static thread_local char buffer[32];

static inline const char *toChars(int value) {
  std::sprintf(buffer, "%d", value);
  return buffer;
}
static inline const char *toChars(long value) {
  std::sprintf(buffer, "%ld", value);
  return buffer;
}
static inline const char *toChars(long long value) {
  std::sprintf(buffer, "%lld", value);
  return buffer;
}
static inline const char *toChars(unsigned value) {
  std::sprintf(buffer, "%u", value);
  return buffer;
}
static inline const char *toChars(unsigned long value) {
  std::sprintf(buffer, "%lu", value);
  return buffer;
}
static inline const char *toChars(unsigned long long value) {
  std::sprintf(buffer, "%llu", value);
  return buffer;
}
static inline const char *toChars(float value) {
  std::sprintf(buffer, "%f", value);
  return buffer;
}
static inline const char *toChars(double value) {
  std::sprintf(buffer, "%f", value);
  return buffer;
}

static bool StringEqualIgnoreCase(const string &lhs, const string &rhs) {
  if (lhs.size() != rhs.size())
    return false;
  const char *p1 = lhs.c_str();
  const char *p2 = rhs.c_str();
  for (size_t i = lhs.size(); i > 0; --i, p1++, p2++) {
    if (toupper(*p1) != toupper(*p2))
      return false;
  }

  return true;
}
} // namespace storage
