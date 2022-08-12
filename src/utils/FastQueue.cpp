#include "FastQueue.h"

namespace storage {
thread_local vector<InnerQueue *> _localInner(_MAX_QUEUE_COUNT, nullptr);
atomic_int32_t _queueCount{0};
} // namespace storage