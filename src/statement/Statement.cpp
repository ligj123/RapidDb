#include "Statement.h"

#include "../core/LeafRecord.h"

namespace storage {

void Statement::AddLeafRecord(LeafRecord *lr) {
  if (lr->GetLock()->GetRecordResult() == RecordResult::INIT) {
    _lstWaitRecord.push_back(lr);
  } else {
    _lstFinshRecord.push_back(lr);
    if (lr->GetLock()->GetRecordResult() == RecordResult::ERROR) {
      _stmtResult->_vctError.push_back(
          move(lr->GetLock()->_errMsg->GetErrorMsg()));
      _stmtFailed.store(true, memory_order_relaxed);
    }
  }
}
} // namespace storage
