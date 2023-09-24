#pragma once
#include "ParserResult.h"

namespace storage {
class Parser {
public:
  Parser() = delete;
  static bool Parse(const MString &sql, SQLParserResult &result);
  static bool Tokenize(const MString &sql, MVector<int16_t> &vctToken);
};
} // namespace storage