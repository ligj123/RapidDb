#pragma once
#include "../src/header.h"
#include <stdint.h>

namespace storage {
void MutexTest();
void ArrayTest();
void TablePointTest(uint16_t userThreads, uint16_t poolThreads,
                    uint16_t sessGroupNum, uint16_t sessTaskNum, int sessionNum,
                    int rowNum, int totalOpTimes);
// void InsertSpeedPrimaryTest(uint64_t row_count);
// void InsertSpeedUniqueTest(uint64_t row_count);
// void InsertSpeedNonUniqueTest(uint64_t row_count);
// void MultiThreadInsertSpeedPrimaryTest(int threadCount, uint64_t row_count);
} // namespace storage
