#pragma once
#include "../src/header.h"
#include <stdint.h>

namespace storage {
void MutexTest();
void ArrayTest();
void InsertSpeedPrimaryTest(uint64_t row_count);
void InsertSpeedUniqueTest(uint64_t row_count);
void InsertSpeedNonUniqueTest(uint64_t row_count);
void MultiThreadInsertSpeedPrimaryTest(int threadCount, uint64_t row_count);
} // namespace storage
