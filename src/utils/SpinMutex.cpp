#include "SpinMutex.h"

namespace storage {
thread_local uint32_t g_threadId = get_thread_id();
}