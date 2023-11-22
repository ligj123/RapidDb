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
void TestSingleQueue(uint64_t count);
void TestSTQueue(uint64_t count);
void TestFastQueue(uint16_t threads, uint64_t count);
} // namespace storage
