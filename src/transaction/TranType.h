#pragma once
#include <ostream>

namespace storage {
enum class TranType : uint8_t {
  /**Automated create a transaction for a single task*/
  AUTOMATE = 0,
  /**Manual create a transaction. The tasks will be executed after committed*/
  MANUAL_DELAY,
  /**Manual create a transaction, execute the tasks at once after add */
  MANUAL_ATONCE
};

inline std::ostream &operator<<(std::ostream &os, const TranType &type) {
  os << "TranType::";
  switch (type) {
  case TranType::AUTOMATE:
    os << "AUTOMATE(" << (int)TranType::AUTOMATE << ")";
    break;
  case TranType::MANUAL_DELAY:
    os << "MANUAL_DELAY(" << (int)TranType::MANUAL_DELAY << ")";
    break;
  case TranType::MANUAL_ATONCE:
    os << "MANUAL_ATONCE(" << (int)TranType::MANUAL_ATONCE << ")";
    break;
  default:
    os << "UNKNOWN(" << (int)type << ")";
    break;
  }
}
} // namespace storage
