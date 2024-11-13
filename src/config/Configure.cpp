#include "Configure.h"

namespace storage {
Configure *Configure::instance{nullptr};
bool Configure::LoadConfig(const string &cfg) { return true; }
} // namespace storage
