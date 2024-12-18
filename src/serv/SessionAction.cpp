#include "SessionAction.h"

namespace storage {
TaskStatus SessionRecordAction::Exec() {
  assert(_lr->_recLock != nullptr);
  _stmt->AddLeafRecord(_lr);
  return TaskStatus::FINISHED;
}

TaskStatus SessionErrMsgAction::Exec() { return TaskStatus::FINISHED; }

TaskStatus SessionCreateAction::Exec() { return TaskStatus::FINISHED; }

TaskStatus SessionCloseAction::Exec() { return TaskStatus::FINISHED; }
} // namespace storage