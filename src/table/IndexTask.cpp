#include "TableTask.h"

namespace storage {
void TableSingleTask::Run() {}

bool TableSingleTask::AddStatement(uint16_t tid, Statement *stmt,
                                   bool bSubmit) {
  if (_table->GetTableStatus() == ResStatus::Obsolete)
    return false;

  _fqStmt.Push(tId, stmt, bSubmit);
  return true;
}

bool TableSingleTask::AddLeafRecord(uint16_t tId, LeafRecord *lr,
                                    bool bSubmit) {
  if (_table->GetTableStatus() == ResStatus::Obsolete)
    return false;

  _fqRecord.Push(tId, lr, bSubmit);
  return true;
}

} // namespace storage