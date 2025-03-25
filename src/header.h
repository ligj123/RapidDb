#pragma once
#include <stdint.h>

typedef unsigned char Byte;
typedef uint32_t PageID;
typedef uint32_t StmtID; // Statement ID
typedef uint64_t VersionStamp;
typedef uint64_t TranID;
// Datatime: the elapsing microseconds since epoch
typedef uint64_t DT_MicroSec;
// Datatime: the elapsing milliseconds since epoch
typedef uint64_t DT_MilliSec;
// Datatime: the elapsing seconds since epoch
typedef uint64_t DT_Second; // seconds
const Byte VALUE_TYPE = 0x80;
const Byte DATE_TYPE = 0x7F;
const int UI16_LEN = sizeof(uint16_t);
const int UI32_LEN = sizeof(uint32_t);
const int UI64_LEN = sizeof(uint64_t);
const int UI16_2_LEN = sizeof(uint16_t) * 2;
const int UI16_3_LEN = sizeof(uint16_t) * 3;
const int BYTE_SIZE = 8;
const int PAGE_ID_LEN = sizeof(PageID);
const uint64_t NANO_SEC = 1000000000;
/**Invalid page id = UINT32_MAX*/
const PageID PAGE_NULL_POINTER = UINT32_MAX;
const TranID TXID_NULL = UINT64_MAX;

#define DEFAULT_MAX_THREADS 8
#define SINGLE_VERSION
#define NO_WRITE_DISK // Only for some press testcases

extern const char *HexStr[];
