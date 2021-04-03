#pragma once
#include <ostream>

namespace storage {
enum class ActionType : uint8_t { UNKNOWN = 0, INSERT, UPDATE, DELETE, QUERY };

inline std::ostream &operator<<(std::ostream &os, const ActionType &type) {
  switch (type) {
  case ActionType::INSERT:
    os << "INSERT(" << (int)ActionType::INSERT << ")";
    break;
  case ActionType::UPDATE:
    os << "UPDATE(" << (int)ActionType::UPDATE << ")";
    break;
  case ActionType::DELETE:
    os << "DELETE(" << (int)ActionType::DELETE << ")";
    break;
  case ActionType::QUERY:
    os << "QUERY(" << (int)ActionType::QUERY << ")";
    break;
  case ActionType::UNKNOWN:
  default:
    os << "UNKNOWN(" << (int)ActionType::UNKNOWN << ")";
    break;
  }

  return os;
}
} // namespace storage
