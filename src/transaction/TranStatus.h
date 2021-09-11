#pragma once
#include <ostream>

namespace storage {
enum class TranStatus : uint8_t {
  // Just create the transaction and has not following action
  CREATED = 0,
  // Already commit, the tasks are running.
  COMMITTING,
  // User stoped the transaction before it finished
  ROLLBACK,
  // Due to data or other error, the transaction failed.
  FAILED,
  // Exceed the deadline time before commit or not finish.
  TIMEOUT,
  // All tasks finished and the log write to disk
  COMMITTED
};

inline std::ostream &operator<<(std::ostream &os, const TranStatus &status) {
  os << "TranStatus::";
  switch (status) {
  case TranStatus::CREATED:
    os << "CREATED(" << (int)TranStatus::CREATED << ")";
    break;
  case TranStatus::COMMITTING:
    os << "COMMITTING(" << (int)TranStatus::COMMITTING << ")";
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
  case TranStatus::COMMITTED:
    os << "COMMITTED(" << (int)TranStatus::COMMITTED << ")";
    break;
  default:
    os << "UNKNOWN(" << (int)status << ")";
    break;
  }
}
} // namespace storage
