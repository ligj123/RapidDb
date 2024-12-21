#include "../header.h"
#include <ostream>

namespace storage {
enum class CcProtocol : uint8_t {
  OCC = 0, // Optimistic Read + Optimistic Write
  OccRead, // Optimistic Read + Pessimistic Write
  Locking, // Pessimistic Read + Pessimistic Write
};

enum class IsoLevel : uint8_t {
  ReadUncommited = 0,
  ReadCommited,
  RepeatableRead,
  Serializable
};

enum class TranStatus : uint8_t {
  INIT = 0,  // Just create the instance
  IN_TRAN,   // Start a new transaction with BEGIN command, NOT auto commit
             // transaction
  AUTO_TRAN, // Auto commit transaction, a statement start a transaction and
             // commit at the statement end.
  FINISHED,  // The transaction has end with command COMMIT or ABORT
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
  os << "IsoLevel::";
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
    assert(false);
    os << "UNKNOWN(" << (int)level << ")";
    break;
  }

  return os;
}

inline std::ostream &operator<<(std::ostream &os, const TranStatus &status) {
  os << "TranStatus::";
  switch (status) {
  case TranStatus::INIT:
    os << "INIT(" << (int)TranStatus::INIT << ")";
    break;
  case TranStatus::IN_TRAN:
    os << "IN_TRAN(" << (int)TranStatus::IN_TRAN << ")";
    break;
  case TranStatus::FINISHED:
    os << "FINISHED(" << (int)TranStatus::FINISHED << ")";
    break;
  case TranStatus::AUTO_TRAN:
    os << "AUTO_TRAN(" << (int)TranStatus::AUTO_TRAN << ")";
    break;
  default:
    assert(false);
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
    assert(false);
    os << "UNKNOWN(" << (int)type << ")";
    break;
  }

  return os;
}

} // namespace storage