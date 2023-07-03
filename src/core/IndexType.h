#pragma once
#include <ostream>

namespace storage {
enum class IndexType : int8_t {
  UNKNOWN = 0,
  PRIMARY,
  HIDE_PRIMARY,
  UNIQUE,
  NON_UNIQUE
};

inline std::ostream &operator<<(std::ostream &os, const IndexType &it) {
  switch (it) {
  case IndexType::PRIMARY:
    os << "PRIMARY(" << (int)IndexType::PRIMARY << ")";
    break;
  case IndexType::HIDE_PRIMARY:
    os << "HIDEPRIMARY(" << (int)IndexType::HIDE_PRIMARY << ")";
    break;
  case IndexType::UNIQUE:
    os << "UNIQUE(" << (int)IndexType::UNIQUE << ")";
    break;
  case IndexType::NON_UNIQUE:
    os << "NON_UNIQUE(" << (int)IndexType::NON_UNIQUE << ")";
    break;
  case IndexType::UNKNOWN:
  default:
    os << "UNKNOWN(" << (int)IndexType::UNKNOWN << ")";
    break;
  }

  return os;
}
} // namespace storage