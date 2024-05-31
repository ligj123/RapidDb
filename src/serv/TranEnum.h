#include "../header.h"
#include <ostream>

namespace storage {
enum class IsoLevel : uint8_t {
  ReadUncommited = 0,
  ReadCommited,
  RepeatableRead,
  Serializable
};

enum class TranStatus : uint8_t {
  // Just create the transaction and has not following action
  CREATED = 0,
  // Already submit commit, the tasks are running.
  COMMITTING,
  // User stoped the transaction before it finished
  ROLLBACK,
  // Due to data or other error, the transaction failed.
  FAILED,
  // Exceed the deadline time before commit or not finish.
  TIMEOUT,
  // All tasks finished and the log write to disk, and all resource will be
  // released.
  CLOSED
};

enum class TranType : uint8_t {
  /**Automated create a transaction for a single task*/
  AUTOMATE = 0,
  /**Manual create a transaction. The tasks will be executed after committed*/
  MANUAL_DELAY,
  /**Manual create a transaction, execute the tasks at once after add the
     transaction*/
  MANUAL_ATONCE
};

inline std::ostream &operator<<(std::ostream &os, const IsoLevel &level) {
  os << "TranStatus::";
  switch (level) {
  case IsoLevel::ReadUncommited:
    os << "ReadUncommited(" << (int)IsoLevel::ReadUncommited << ")";
    break;
  case IsoLevel::ReadCommited:
    os << "ReadCommited(" << (int)IsoLevel::ReadCommited << ")";
    break;
  case IsoLevel::RepeatableRead:
    os << "RepeatableRead(" << (int)IsoLevel::RepeatableRead << ")";
    break;
  case IsoLevel::Serializable:
    os << "Serializable(" << (int)IsoLevel::Serializable << ")";
    break;
  default:
    os << "UNKNOWN(" << (int)level << ")";
    break;
  }

  return os;
}

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
  case TranStatus::CLOSED:
    os << "CLOSED(" << (int)TranStatus::CLOSED << ")";
    break;
  default:
    os << "UNKNOWN(" << (int)status << ")";
    break;
  }

  return os;
}

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

  return os;
}

} // namespace storage