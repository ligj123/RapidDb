#pragma once
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

#ifdef STD_MEM
#define BytesCopy(dst, src, count) std::memcpy(dst, src, count)
#else
#define BytesCopy(dst, src, count) BytesCpy(dst, src, count)
#endif // STD_MEM