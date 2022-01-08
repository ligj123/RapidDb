#pragma once
#ifdef _MSVC_LANG
#include <stdlib.h>
#define BytesSwap16 _byteswap_ushort
#define BytesSwap32 _byteswap_ulong
#define BytesSwap64 _byteswap_uint64
#else
#include <byteswap.h>
#define BytesSwap16 bswap_16
#define BytesSwap32 bswap_32
#define BytesSwap64 bswap_64
#endif // _MSVC_LANG