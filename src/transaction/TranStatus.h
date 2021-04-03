#pragma once
#include <ostream>

namespace storage {
enum class TranStatus {
  CREATED = 0,
  COMMITTED,
  ROLLBACK,
  FAILED,
  TIMEOUT,
  CLOSED
};

inline std::ostream &operator<<(std::ostream &os, const TranStatus &status) {
  os << "TranStatus::";
  switch (status) {
  case TranStatus::CREATED:
    os << "CREATED(" << (int)TranStatus::CREATED << ")";
    break;
  case TranStatus::COMMITTED:
    os << "COMMITTED(" << (int)TranStatus::COMMITTED << ")";
    break;
  case TranStatus::ROLLBACK:
    os << "ROLLBACK(" << (int)TranStatus::ROLLBACK << ")";
    break;
  case TranStatus::FAILED:
    os << "FAILED(" << (int)TranStatus::FAILED << ")";
    break;
  case TranStatus::TIMEOUT:
    os << "TIMEOUT(" << (int)TranStatus::TIMEOUT << ")";
    break;
  case TranStatus::CLOSED:
    os << "CLOSED(" << (int)TranStatus::CLOSED << ")";
    break;
  default:
    os << "UNKNOWN(" << (int)status << ")";
    break;
  }
}
} // namespace storage
