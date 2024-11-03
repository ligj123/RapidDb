#include "IndexTask.h"

namespace storage {
TaskStatus PriIndexTask::Run() { return TaskStatus::RUNNING; }

TaskStatus SecIndexTask::Run() { return TaskStatus::RUNNING; }

} // namespace storage