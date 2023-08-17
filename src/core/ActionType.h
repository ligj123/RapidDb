#pragma once
#include <ostream>

namespace storage {
enum class ActionType : uint8_t {
  UNKNOWN = 0,
  INSERT,       // Insert the LeafRecord
  UPDATE,       // Update the LeafRecord
  DELETE,       // Delete the LeafRecord
  QUERY,        // Only query and return the record without following action
  QUERY_SHARE,  // Query and lock this record for read
  QUERY_UPDATE, // Query and return the record and add write lock for it
  UPSERT        // Insert if not exist or update if exist
};

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
  case ActionType::QUERY_SHARE:
    os << "QUERY_SHARE(" << (int)ActionType::QUERY_SHARE << ")";
    break;
  case ActionType::QUERY_UPDATE:
    os << "QUERY_UPDATE(" << (int)ActionType::QUERY_UPDATE << ")";
    break;
  case ActionType::UPSERT:
    os << "UPSERT(" << (int)ActionType::UPSERT << ")";
    break;
  case ActionType::UNKNOWN:
  default:
    os << "UNKNOWN(" << (int)ActionType::UNKNOWN << ")";
    break;
  }

  return os;
}
} // namespace storage
