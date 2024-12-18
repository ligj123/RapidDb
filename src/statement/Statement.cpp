#include "Statement.h"

#include "../core/LeafRecord.h"

namespace storage {

void Statement::AddLeafRecord(LeafRecord *lr) {
  if (lr->GetLock()->GetRecordResult() == RecordResult::INIT) {
    _lstWaitRecord.push_back(lr);
  } else {
    _lstFinshRecord.push_back(lr);
    if (lr->GetLock()->GetRecordResult() == RecordResult::ERROR) {
      _stmtFailed = true;
    }
  }
}
} // namespace storage
