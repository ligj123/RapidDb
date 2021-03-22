#include "PressTest.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

namespace storage {
using namespace std;
static int64_t times = 10000000;
static int map_size = 1000;

int BytesCompare(char *bys1, uint32_t len1, char *bys2, uint32_t len2) {
  // int minLen = min(len1, len2);

  // for (int i = 0; i < minLen; i++) {
  //   int a = (unsigned char)bys1[i];
  //   int b = (unsigned char)bys2[i];
  //   if (a != b) {
  //     return a - b;
  //   }
  // }

  // return len1 - len2;

  int hr = memcmp(bys1, bys2, std::min(len1, len2));
  if (hr != 0)
    return hr;
  return len1 - len2;
}

void Int64ToBytes(int64_t val, char *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  if (bkey)
    val ^= 0x8000000000000000ll;
  *((int64_t *)pArr) = val;
#else
  if (bkey) {
    val ^= 0x8000000000000000ll;
    char *buf = (char *)&val;
    pArr[0] = buf[7];
    pArr[1] = buf[6];
    pArr[2] = buf[5];
    pArr[3] = buf[4];
    pArr[4] = buf[3];
    pArr[5] = buf[2];
    pArr[6] = buf[1];
    pArr[7] = buf[0];
  } else {
    *((int64_t *)pArr) = val;
  }
#endif
}

int SearchVector(char **ppAr, int len, char *pp) {
  int end = len - 1;
  int start = 0;
  while (true) {
    if (start > end) {
      return start;
    }

    int middle = (start + end) / 2;
    int hr = BytesCompare(ppAr[middle], 8, pp, 8);

    if (hr < 0) {
      start = middle + 1;
    } else if (hr > 0) {
      end = middle - 1;
    } else {
      return middle;
    }
  }
}

int64_t Int64FromBytes(char *pArr, bool bkey = false) {
  int64_t val = 0;
#ifdef BIGENDIAN
  val = *((int64_t *)pArr);
  if (bkey)
    val ^= 0x8000000000000000ll;
#else
  if (bkey) {
    char *buf = (char *)&val;
    buf[7] = pArr[0];
    buf[6] = pArr[1];
    buf[5] = pArr[2];
    buf[4] = pArr[3];
    buf[3] = pArr[4];
    buf[2] = pArr[5];
    buf[1] = pArr[6];
    buf[0] = pArr[7];
    val ^= 0x8000000000000000ll;
  } else {
    val = *((int64_t *)pArr);
  }
#endif

  return val;
}

void ArrayTest() {
  int64_t count_size = 0;
  char **ppAr = new char *[map_size];
  char *buf = new char[map_size * 8];

  chrono::system_clock::time_point st = chrono::system_clock::now();
  for (int i = 0; i < times; i++) {
    int64_t num = i;
    int64_t by1 = num & 0xff;
    int64_t by2 = (num >> 8) & 0xff;
    int64_t by3 = (num >> 16) & 0xff;
    int64_t by4 = (num >> 24) & 0xff;
    int64_t priKey = (((by1 & 0x55) + (by4 & 0xAA)) << 24) +
                     (((by2 & 0x55) + (by3 & 0xAA)) << 16) +
                     (((by2 & 0xAA) + (by3 & 0x55)) << 8) +
                     (((by1 & 0xAA) + (by4 & 0x55)));

    int off = (i % map_size) * 8;
    Int64ToBytes(priKey, buf + off, true);
    int pos = SearchVector(ppAr, i % map_size, buf + off);
    memcpy(&ppAr[pos + 1], &ppAr[pos], (i % map_size - pos) * 8);
    *(ppAr + pos) = (buf + off);
    // count_size++;
  }

  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  cout << "T Time(ms):" << duration.count() << "\tCount:" << count_size << endl;

  // for (int i = 0; i < map_size; i++) {
  //  std::cout << Int64FromBytes(ppAr[i], true) << "\n";
  //}

  // std::cout << std::endl;
}
} // namespace storage