#pragma once
#include "../header.h"
#include "Log.h"
#include <cstdint>
#include <cstring>

namespace utils {
inline int64_t Int64FromBytes(Byte *pArr, bool bkey = false) {
  int64_t val = 0;
#ifdef BIGENDIAN
  val = *((int64_t *)pArr);
  if (bkey)
    val ^= 0x8000000000000000ll;
#else
  if (bkey) {
    Byte *buf = (Byte *)&val;
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

inline void Int64ToBytes(int64_t val, Byte *pArr, bool bkey = false) {
#ifdef BIGENDIAN
  if (bkey)
    val ^= 0x8000000000000000ll;
  *((int64_t *)pArr) = val;
#else
  if (bkey) {
    val ^= 0x8000000000000000ll;
    Byte *buf = (Byte *)&val;
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

inline uint64_t UInt64FromBytes(Byte *pArr, bool bkey = false) {
  uint64_t val = 0;
#ifdef BIGENDIAN
  val = *((uint64_t *)pArr);
#else
  if (bkey) {
    Byte *buf = (Byte *)&val;
    buf[7] = pArr[0];
    buf[6] = pArr[1];
    buf[5] = pArr[2];
    buf[4] = pArr[3];
    buf[3] = pArr[4];
    buf[2] = pArr[5];
    buf[1] = pArr[6];
    buf[0] = pArr[7];
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
    Byte *buf = (Byte *)&val;
    pArr[0] = buf[7];
    pArr[1] = buf[6];
    pArr[2] = buf[5];
    pArr[3] = buf[4];
    pArr[4] = buf[3];
    pArr[5] = buf[2];
    pArr[6] = buf[1];
    pArr[7] = buf[0];
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
    Byte *buf = (Byte *)&val;
    buf[3] = pArr[0];
    buf[2] = pArr[1];
    buf[1] = pArr[2];
    buf[0] = pArr[3];
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
    Byte *buf = (Byte *)&val;
    pArr[0] = buf[3];
    pArr[1] = buf[2];
    pArr[2] = buf[1];
    pArr[3] = buf[0];
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
    Byte *buf = (Byte *)&val;
    buf[3] = pArr[0];
    buf[2] = pArr[1];
    buf[1] = pArr[2];
    buf[0] = pArr[3];
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
    Byte *buf = (Byte *)&val;
    pArr[0] = buf[3];
    pArr[1] = buf[2];
    pArr[2] = buf[1];
    pArr[3] = buf[0];
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
    Byte *buf = (Byte *)&val;
    buf[1] = pArr[0];
    buf[0] = pArr[1];
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
    Byte *buf = (Byte *)&val;
    pArr[0] = buf[1];
    pArr[1] = buf[0];
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
    Byte *buf = (Byte *)&val;
    buf[1] = pArr[0];
    buf[0] = pArr[1];
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
    Byte *buf = (Byte *)&val;
    pArr[0] = buf[1];
    pArr[1] = buf[0];
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
    Byte *buf = (Byte *)&val;
    buf[7] = pArr[0];
    buf[6] = pArr[1];
    buf[5] = pArr[2];
    buf[4] = pArr[3];
    buf[3] = pArr[4];
    buf[2] = pArr[5];
    buf[1] = pArr[6];
    buf[0] = pArr[7];
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
    Byte *buf = (Byte *)p;
    pArr[0] = buf[7];
    pArr[1] = buf[6];
    pArr[2] = buf[5];
    pArr[3] = buf[4];
    pArr[4] = buf[3];
    pArr[5] = buf[2];
    pArr[6] = buf[1];
    pArr[7] = buf[0];
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
    Byte *buf = (Byte *)&val;
    buf[3] = pArr[0];
    buf[2] = pArr[1];
    buf[1] = pArr[2];
    buf[0] = pArr[3];
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
    Byte *buf = (Byte *)p;
    pArr[0] = buf[3];
    pArr[1] = buf[2];
    pArr[2] = buf[1];
    pArr[3] = buf[0];
  } else {
    *((uint32_t *)pArr) = *p;
  }
#endif
}

inline int BytesCompare(Byte *bys1, uint32_t len1, Byte *bys2, uint32_t len2) {
  int hr = std::memcmp(bys1, bys2, std::min(len1, len2));
  if (hr != 0)
    return hr;
  return len1 - len2;

  // int minLen = std::min(len1, len2);
  // for (int i = 0; i < minLen; i++) {
  //  int a = (unsigned char)bys1[i];
  //  int b = (unsigned char)bys2[i];
  //  if (a != b) {
  //    return a - b;
  //  }
  //}
  // return len1 - len2;
}
} // namespace utils
