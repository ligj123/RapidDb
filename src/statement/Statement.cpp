#include "Statement.h"

#include "../core/LeafRecord.h"

namespace storage {

void Statement::AddLeafRecord(LeafRecord *lr) {
  if (lr->GetLock()->GetRecordResult() == RecordResult::INIT) {
    _lstWaitRecord.push_back(lr);
  } else {
    _lstFinishRecord.push_back(lr);
    if (lr->GetLock()->GetRecordResult() == RecordResult::ERROR) {
      _stmtResult->_vctError.push_back(
          move(lr->GetLock()->_errMsg->GetErrorMsg()));
      _stmtFailed.store(true, memory_order_relaxed);
    }
  }
}

std::ostream &operator<<(std::ostream &os, const StmtStatus &s) {
  os << "StmtStatus::";
  switch (s) {
  case StmtStatus::Created:
    os << "Created(" << (int)StmtStatus::Created << ")";
    break;
  case StmtStatus::Executing:
    os << "Executing(" << (int)StmtStatus::Executing << ")";
    break;
  case StmtStatus::Executed:
    os << "Executed(" << (int)StmtStatus::Executed << ")";
    break;
  case StmtStatus::Logging:
    os << "Logging(" << (int)StmtStatus::Logging << ")";
    break;
  case StmtStatus::Logged:
    os << "Logged(" << (int)StmtStatus::Logged << ")";
    break;
  case StmtStatus::Finished:
    os << "Finished(" << (int)StmtStatus::Finished << ")";
    break;
  }

  return os;
}
} // namespace storage
