#pragma once
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include <chrono>
#include <regex>
#include <string>

namespace utils {
static const char *NAME_PATTERN =
    "^[_a-zA-Z\\u4E00-\\u9FA5][_a-zA-Z0-9\\u4E00-\\u9FA5]*?$";
static regex reg(NAME_PATTERN);

static void IsValidName(string name) {
  cmatch mt;
  if (!regex_match(name.c_str(), mt, reg)) {
    throw utils::ErrorMsg(storage::TB_INVALID_FILE_VERSION, {name});
  }
}

static uint64_t MSTime() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

static std::string StrMSTime() {
  return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count());
}
} // namespace utils
