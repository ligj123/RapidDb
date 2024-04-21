#pragma once
#include "../cache/Mallocator.h"
#include "../statement/Statement.h"
#include "../utils/Utilitys.h"

namespace storage {

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

struct Transaction {
  void Reset(uint64_t tid) {
    assert(_vctStatement.size() == 0);
    _tid = tid;
    _createTime = utils::MicroSecTime();
    _stopTime = UINT64_MAX;
  }

public:
  uint64_t _tid{UINT64_MAX};
  // Create time
  DT_MicroSec _createTime{UINT64_MAX};
  // The finished or abort time to execute for this statement
  DT_MicroSec _endTime{UINT64_MAX};
  // The statements executed in this transaction
  MVector<Statement *> _vctStatement;
  // spin lock
  SpinMutex _spinMutex;
  // The session own this transaction
  Session *_session;

  TranStatus _tranStatus;
  TranType _tranType;
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