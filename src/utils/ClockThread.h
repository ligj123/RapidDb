#pragma once

#include "../cache/Mallocator.h"
#include "ErrorMsg.h"
#include "SpinMutex.h"
#include <condition_variable>
#include <deque>
#include <future>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>

namespace storage {
using namespace std;
class ClockThread {
public:
protected:
};
} // namespace storage