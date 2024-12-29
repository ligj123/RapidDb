#pragma once
#include <ostream>

namespace storage {
enum class LogType : uint8_t {
  UNKNOWN = 0,
  RECORD,     // Insert, update or delete a record
  PAGE_SPLIT, // Split a IndexPage
  TABLE_OP,
  DATABASE_OP
};

inline std::ostream &operator<<(std::ostream &os, const LogType &type) {
  switch (type) {
  case LogType::RECORD:
    os << "RECORD(" << (int)LogType::RECORD << ")";
    break;
  case LogType::PAGE_SPLIT:
    os << "PAGE_SPLIT(" << (int)LogType::PAGE_SPLIT << ")";
    break;
  case LogType::TABLE_OP:
    os << "TABLE_OP(" << (int)LogType::TABLE_OP << ")";
    break;
  case LogType::TABLE_DELETE:
    os << "TABLE_DELETE(" << (int)LogType::TABLE_DELETE << ")";
    break;
  case LogType::DATABASE_OP:
    os << "DATABASE_OP(" << (int)LogType::DATABASE_OP << ")";
    break;
  case LogType::UNKNOWN:
    os << "UNKNOWN(" << (int)LogType::UNKNOWN << ")";
    break;
  default:
    os << "ERROR LogType(" << (int)type << ")";
    break;
  }

  return os;
}
} // namespace storage