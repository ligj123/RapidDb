#pragma once
#include "../src/header.h"
#include <stdint.h>

namespace storage {
void MutexTest();
void ArrayTest();
void TablePointTest(uint16_t userThreads, uint16_t poolThreads,
                    uint16_t sessGroupNum, uint16_t sessTaskNum, int sessionNum,
                    int rowNum, int totalOpTimes, bool bExclusive);
void MultiThreadInsertTest(int threadNum, int rowNum);
void TestTableSpeed(int userThreads, int tblThreads, int rowNum,
                    int totalOpTimes);
void TestMultiTable(int tblNum, int sessGroupNum, int sessNum, int rowNum,
                    int totalOpTimes);
} // namespace storage
