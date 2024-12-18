#include "Statement.h"

#include "../core/LeafRecord.h"

namespace storage {

void Statement::AddLeafRecord(LeafRecord *lr) {
  if (lr->GetLock()->GetRecordResult() == RecordResult::INIT) {
    _qWaitRecord.push_back(lr);
  } else {
    _qFinshRecord.push_back(lr);
    if (lr->GetLock()->GetRecordResult() == RecordResult::ERROR) {
      _stmtFailed = true;
    }
  }
}
} // namespace storage
