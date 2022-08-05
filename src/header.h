﻿#pragma once
#include <stdint.h>
using namespace std;
#define _DEBUG_TEST
#define _MAX_QUEUE_COUNT 8
typedef unsigned char Byte;
typedef uint32_t PageID;
typedef uint64_t VersionStamp;
typedef uint64_t DT_MicroSec; // microseconds
typedef uint64_t DT_MilliSec; // milliseconds
typedef uint64_t DT_Second;   // seconds
const Byte VALUE_TYPE = 0x80;
const Byte DATE_TYPE = 0x7F;
const int UI16_LEN = sizeof(uint16_t);
const int UI32_LEN = sizeof(uint32_t);
const int UI64_LEN = sizeof(uint64_t);
const int UI16_2_LEN = sizeof(uint16_t) * 2;
const int UI16_3_LEN = sizeof(uint16_t) * 3;
const int BYTE_SIZE = 8;
const uint64_t NANO_SEC = 1000000000;
/**Invalid page id = UINT32_MAX*/
const PageID PAGE_NULL_POINTER = UINT32_MAX;