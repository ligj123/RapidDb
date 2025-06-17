#pragma once
#include <ostream>

namespace storage {
enum class LogType : uint8_t {
  UNKNOWN = 0,
  DML_OP,     // DML operation, Insert, update or delete the records
  PAGE_SPLIT, // Split an IndexPage, inlude alloc new pages
  DDL_OP      // DML operation, create, drop database or tables.
};

inline std::ostream &operator<<(std::ostream &os, const LogType &type) {
  switch (type) {
  case LogType::DML_OP:
    os << "DML_OP(" << (int)LogType::DML_OP << ")";
    break;
  case LogType::PAGE_SPLIT:
    os << "PAGE_SPLIT(" << (int)LogType::PAGE_SPLIT << ")";
    break;
  case LogType::DDL_OP:
    os << "DDL_OP(" << (int)LogType::DDL_OP << ")";
    break;
  default:
    os << "ERROR LogType(" << (int)type << ")";
    break;
  }

  return os;
}
} // namespace storage