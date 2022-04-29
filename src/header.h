#pragma once
#include <stdint.h>
using namespace std;

typedef unsigned char Byte;
typedef uint32_t PageID;
typedef uint64_t VersionStamp;
typedef uint64_t DT_MicroSec; // microseconds
typedef uint64_t DT_MilliSec; // milliseconds
const int VALUE_TYPE = 0x80;
const int DATE_TYPE = 0x7F;
const int UI16_LEN = sizeof(uint16_t);
const int UI32_LEN = sizeof(uint32_t);
const int UI64_LEN = sizeof(uint64_t);
const int UI16_2_LEN = sizeof(uint16_t) * 2;
const int UI16_3_LEN = sizeof(uint16_t) * 3;
const int BYTE_SIZE = 8;
const uint64_t NANO_SEC = 1000000000;
