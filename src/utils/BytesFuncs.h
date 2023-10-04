#pragma once
#include "../header.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

#ifdef BIGENDIAN
#define BytesSwap16(a) a
#define BytesSwap32(a) a
#define BytesSwap64(a) a
#else
#ifdef _MSVC_LANG
#include <stdlib.h>
#define BytesSwap16(a) _byteswap_ushort(a)
#define BytesSwap32(a) _byteswap_ulong(a)
#define BytesSwap64(a) _byteswap_uint64(a)
#else
#include <byteswap.h>
#define BytesSwap16(a) bswap_16(a)
#define BytesSwap32(a) bswap_32(a)
#define BytesSwap64(a) bswap_64(a)
#endif // _MSVC_LANG
#endif // BIGENDIAN

namespace storage {
inline int64_t Int64FromBytes(Byte *pArr, bool bkey = false) {
  int64_t val = 0;
#ifdef BIGENDIAN
  val = *((int64_t *)pArr);
  if (bkey)
    val ^= 0x8000000000000000ll;
#else
  if (bkey) {
    val = BytesSwap64(*(int64_t *)pArr);
    val ^= 0x8000000000000000ll;
  } else {
    val = *((int64_t *)pArr);
  }
#endif

  return val;
}

inline void Int64ToBytes(int64_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  if (bkey)
    val ^= 0x8000000000000000ll;
  *((int64_t *)pArr) = val;
#else
  if (bkey) {
    val ^= 0x8000000000000000ll;
    *(int64_t *)pArr = BytesSwap64(val);
  } else {
    *((int64_t *)pArr) = val;
  }
#endif
}

inline uint64_t UInt64FromBytes(Byte *pArr, bool bkey = false) {
  uint64_t val = 0;
#ifdef BIGENDIAN
  val = *((uint64_t *)pArr);
#else
  if (bkey) {
    val = BytesSwap64(*(uint64_t *)pArr);
  } else {
    val = *((uint64_t *)pArr);
  }
#endif
  return val;
}

inline void UInt64ToBytes(uint64_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  *((uint64_t *)pArr) = val;
#else
  if (bkey) {
    *(uint64_t *)pArr = BytesSwap64(val);
  } else {
    *((uint64_t *)pArr) = val;
  }
#endif
}

inline int32_t Int32FromBytes(Byte *pArr, bool bkey = false) {
  int32_t val = 0;
#ifdef BIGENDIAN
  val = *((int32_t *)pArr);
  if (bkey)
    val ^= 0x80000000;
#else
  if (bkey) {
    val = BytesSwap32(*(int32_t *)pArr);
    val ^= 0x80000000;
  } else {
    val = *((int32_t *)pArr);
  }
#endif
  return val;
}

inline void Int32ToBytes(int32_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  if (bkey)
    val ^= 0x80000000;
  *((int32_t *)pArr) = val;
#else

  if (bkey) {
    val ^= 0x80000000;
    *(int32_t *)pArr = BytesSwap32(val);
  } else {
    *((int32_t *)pArr) = val;
  }
#endif
}

inline uint32_t UInt32FromBytes(Byte *pArr, bool bkey = false) {
  uint32_t val = 0;
#ifdef BIGENDIAN
  val = *((uint32_t *)pArr);
#else
  if (bkey) {
    val = BytesSwap32(*(uint32_t *)pArr);
  } else {
    val = *((uint32_t *)pArr);
  }
#endif
  return val;
}

inline void UInt32ToBytes(uint32_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  *((uint32_t *)pArr) = val;
#else
  if (bkey) {
    *(uint32_t *)pArr = BytesSwap32(val);
  } else {
    *((uint32_t *)pArr) = val;
  }
#endif
}

inline int16_t Int16FromBytes(Byte *pArr, bool bkey = false) {
  int16_t val = 0;
#ifdef BIGENDIAN
  val = *((int16_t *)pArr);
  if (bkey)
    val ^= 0x8000;
#else
  if (bkey) {
    val = BytesSwap16(*(int16_t *)pArr);
    val ^= 0x8000;
  } else {
    val = *((int16_t *)pArr);
  }
#endif
  return val;
}

inline void Int16ToBytes(int16_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  if (bkey)
    val ^= 0x8000;
  *((int16_t *)pArr) = val;
#else
  if (bkey) {
    val ^= 0x8000;
    *(int16_t *)pArr = BytesSwap16(val);
  } else {
    *((int16_t *)pArr) = val;
  }
#endif
}

inline uint16_t UInt16FromBytes(Byte *pArr, bool bkey = false) {
  uint16_t val = 0;
#ifdef BIGENDIAN
  val = *((uint16_t *)pArr);
#else
  if (bkey) {
    val = BytesSwap16(*(uint16_t *)pArr);
  } else {
    val = *((uint16_t *)pArr);
  }
#endif
  return val;
}

inline void UInt16ToBytes(uint16_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  *((uint16_t *)pArr) = val;
#else
  if (bkey) {
    *(uint16_t *)pArr = BytesSwap16(val);
  } else {
    *((uint16_t *)pArr) = val;
  }
#endif
}

inline int8_t Int8FromBytes(Byte *pArr, bool bkey = false) {
  int8_t val = *pArr;
  if (bkey)
    val ^= 0x80;
  return val;
}

inline void Int8ToBytes(int8_t val, Byte *pArr, bool bkey = false) {
  if (bkey)
    val ^= 0x80;
  pArr[0] = val;
}

inline uint8_t UInt8FromBytes(Byte *pArr, bool bkey = false) { return *pArr; }

inline void UInt8ToBytes(uint8_t val, Byte *pArr, bool bkey = false) {
  *pArr = val;
}

inline double DoubleFromBytes(Byte *pArr, bool bkey = false) {
  uint64_t val = 0;
#ifdef BIGENDIAN
  val = *((uint64_t *)pArr);
  if (bkey) {
    if (val & 0x8000000000000000ll) {
      val ^= 0x8000000000000000ll;
    } else {
      val ^= 0x800fffffffffffffll;
    }
  }
#else
  if (bkey) {
    val = BytesSwap64(*(uint64_t *)pArr);
    if (val & 0x8000000000000000ll) {
      val ^= 0x8000000000000000ll;
    } else {
      val ^= 0x800fffffffffffffll;
    }
  } else {
    val = *((uint64_t *)pArr);
  }
#endif

  return *((double *)&val);
}

inline void DoubleToBytes(double dval, Byte *pArr, bool bkey = false) {
  uint64_t *p = (uint64_t *)&dval;
#ifdef BIGENDIAN
  if (bkey) {
    if (*p & 0x8000000000000000ll) {
      *p ^= 0x800fffffffffffffll;
    } else {
      *p ^= 0x8000000000000000ll;
    }
  }
  *((uint64_t *)pArr) = *p;
#else
  if (bkey) {
    if (*p & 0x8000000000000000ll) {
      *p ^= 0x800fffffffffffffll;
    } else {
      *p ^= 0x8000000000000000ll;
    }
    *(int64_t *)pArr = BytesSwap64(*p);
  } else {
    *((uint64_t *)pArr) = *p;
  }
#endif
}

inline float FloatFromBytes(Byte *pArr, bool bkey = false) {
  uint32_t val = 0;
#ifdef BIGENDIAN
  val = *((uint32_t *)pArr);
  if (bkey) {
    if (val & 0x80000000) {
      val ^= 0x80000000;
    } else {
      val ^= 0x807fffff;
    }
  }
#else
  if (bkey) {
    val = BytesSwap32(*(uint32_t *)pArr);
    if (val & 0x80000000) {
      val ^= 0x80000000;
    } else {
      val ^= 0x807fffff;
    }
  } else {
    val = *((uint32_t *)pArr);
  }
#endif
  return *((float *)&val);
}

inline void FloatToBytes(float dval, Byte *pArr, bool bkey = false) {
  uint32_t *p = (uint32_t *)&dval;
#ifdef BIGENDIAN
  if (bkey) {
    if (*p & 0x80000000) {
      *p ^= 0x807fffff;
    } else {
      *p ^= 0x80000000;
    }
  }
  *((uint32_t *)pArr) = *p;
#else
  if (bkey) {
    if (*p & 0x80000000) {
      *p ^= 0x807fffff;
    } else {
      *p ^= 0x80000000;
    }
    *(int32_t *)pArr = BytesSwap32(*p);
  } else {
    *((uint32_t *)pArr) = *p;
  }
#endif
}

inline int BytesCompare(const Byte *bys1, size_t len1, const Byte *bys2,
                        size_t len2) {
#ifdef STD_MEM
  int hr = std::memcmp(bys1, bys2, std::min(len1, len2));
  if (hr != 0)
    return hr;
  return len1 - len2;
#else
  size_t minLen = std::min(len1, len2);
  size_t min8 = minLen & 0xFFFFFFFFFFFFFFF8;
  size_t i = 0;
  for (; i < min8; i += 8) {
    int64_t hr =
        BytesSwap64(*(uint64_t *)bys1) - BytesSwap64(*(uint64_t *)bys2);
    if (hr != 0)
      return hr > 0 ? 1 : -1;
    bys1 += 8;
    bys2 += 8;
  }

  for (; i < minLen; i++) {
    int hr = *bys1 - *bys2;
    if (hr != 0)
      return hr;
    bys1++;
    bys2++;
  }

  return (int)(len1 - len2);
#endif // STD_MEM
}

#ifdef STD_MEM
#define BytesCopy(dst, src, len) std::memcpy(dst, src, len)
#else
inline void *BytesCopy(void *dst, const void *src, size_t len) {
  size_t len8 = len & 0xFFFFFFFFFFFFFFF8;
  size_t i = 0;
  Byte *pdst = (Byte *)dst;
  const Byte *psrc = (Byte *)src;
  for (; i < len8; i += 8) {
    *((uint64_t *)pdst) = *((uint64_t *)psrc);
    pdst += 8;
    psrc += 8;
  }

  for (; i < len; i++) {
    *pdst++ = *psrc++;
  }

  return dst;
}
#endif // STD_MEM

inline size_t BytesHash(const Byte *bys, size_t len) {
  size_t len8 = len & 0xFFFFFFFFFFFFFFF8;
  size_t i = 0;
  size_t h = 0;
  for (; i < len8; i += 8) {
    h = (h << 8) ^ (*(size_t *)bys);
    bys += 8;
  }

  for (; i < len; i++) {
    h = (h << 1) ^ (*bys);
    bys++;
  }

  return h;
}

inline bool BytesEqual(const Byte *bys1, size_t len1, const Byte *bys2,
                       size_t len2) {
  if (len1 != len2)
    return false;
  size_t len8 = len1 & 0xFFFFFFFFFFFFFFF8;
  size_t i = 0;
  for (; i < len8; i += 8) {
    if (*(uint64_t *)bys1 != *(uint64_t *)bys2)
      return false;

    bys1 += 8;
    bys2 += 8;
  }

  for (; i < len1; i++) {
    if (*bys1 - *bys2)
      return false;

    bys1++;
    bys2++;
  }

  return true;
}
} // namespace storage
