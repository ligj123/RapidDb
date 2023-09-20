#pragma once
#include "../utils/BytesFuncs.h"
#include "DataType.h"
namespace storage {
// From digital to bytes
template <DataType v> class Case_T {
public:
  static inline auto Run() { return nullptr; }
};

template <> class Case_T<DataType::CHAR> {
public:
  static inline auto Run() { return Int8ToBytes; }
};

template <> class Case_T<DataType::SHORT> {
public:
  static inline auto Run() { return Int16ToBytes; }
};

template <> class Case_T<DataType::INT> {
public:
  static inline auto Run() { return Int32ToBytes; }
};

template <> class Case_T<DataType::LONG> {
public:
  static inline auto Run() { return Int64ToBytes; }
};

template <> class Case_T<DataType::BYTE> {
public:
  static inline auto Run() { return UInt8ToBytes; }
};

template <> class Case_T<DataType ::USHORT> {
public:
  static inline auto Run() { return UInt16ToBytes; }
};

template <> class Case_T<DataType::UINT> {
public:
  static inline auto Run() { return UInt32ToBytes; }
};

template <> class Case_T<DataType::ULONG> {
public:
  static inline auto Run() { return UInt64ToBytes; }
};

template <> class Case_T<DataType::BOOL> {
public:
  static inline auto Run() {
    return [](bool val, Byte *pArr, bool bKey) { *pArr = val; };
  }
};

template <> class Case_T<DataType::DATETIME> {
public:
  static inline auto Run() { return UInt64ToBytes; }
};

template <> class Case_T<DataType::FLOAT> {
public:
  static inline auto Run() { return FloatToBytes; }
};

template <> class Case_T<DataType::DOUBLE> {
public:
  static inline auto Run() { return DoubleToBytes; }
};

template <class T, DataType DT>
inline void DigitalToBytes(T val, Byte *pArr, bool bkey = false) {
  auto f = Case_T<DT>::Run();
  f(val, pArr, bkey);
}

// From bytes to digital
template <DataType v> class Case_F {
public:
  static inline auto Run() { return nullptr; }
};

template <> class Case_F<DataType::CHAR> {
public:
  static inline auto Run() { return Int8FromBytes; }
};

template <> class Case_F<DataType::SHORT> {
public:
  static inline auto Run() { return Int16FromBytes; }
};

template <> class Case_F<DataType::INT> {
public:
  static inline auto Run() { return Int32FromBytes; }
};

template <> class Case_F<DataType::LONG> {
public:
  static inline auto Run() { return Int64FromBytes; }
};

template <> class Case_F<DataType::BYTE> {
public:
  static inline auto Run() { return UInt8FromBytes; }
};

template <> class Case_F<DataType::USHORT> {
public:
  static inline auto Run() { return UInt16FromBytes; }
};

template <> class Case_F<DataType::UINT> {
public:
  static inline auto Run() { return UInt32FromBytes; }
};

template <> class Case_F<DataType::ULONG> {
public:
  static inline auto Run() { return UInt64FromBytes; }
};

template <> class Case_F<DataType::BOOL> {
public:
  static inline auto Run() {
    return [](Byte *pArr, bool bKey) { return *pArr; };
  }
};

template <> class Case_F<DataType::DATETIME> {
public:
  static inline auto Run() { return UInt64FromBytes; }
};

template <> class Case_F<DataType::FLOAT> {
public:
  static inline auto Run() { return FloatFromBytes; }
};

template <> class Case_F<DataType::DOUBLE> {
public:
  static inline auto Run() { return DoubleFromBytes; }
};

template <class T, DataType DT>
inline T DigitalFromBytes(Byte *pArr, bool bkey = false) {
  auto f = Case_F<DT>::Run();
  return f(pArr, bkey);
}

// Set min value
template <DataType v> class Case_Min {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Min<DataType::CHAR> {
public:
  static inline auto Run() { return CHAR_MIN; }
};

template <> class Case_Min<DataType::SHORT> {
public:
  static inline auto Run() { return SHRT_MIN; }
};

template <> class Case_Min<DataType::INT> {
public:
  static inline auto Run() { return INT_MIN; }
};

template <> class Case_Min<DataType::LONG> {
public:
  static inline auto Run() { return LLONG_MIN; }
};

template <> class Case_Min<DataType::BYTE> {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Min<DataType::USHORT> {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Min<DataType::UINT> {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Min<DataType::ULONG> {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Min<DataType::BOOL> {
public:
  static inline auto Run() { return false; }
};

template <> class Case_Min<DataType::DATETIME> {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Min<DataType::FLOAT> {
public:
  static inline auto Run() { return -FLT_MAX; }
};

template <> class Case_Min<DataType::DOUBLE> {
public:
  static inline auto Run() { return -DBL_MAX; }
};

template <class T, DataType DT> inline T MinValue() {
  return Case_Min<DT>::Run();
}

// Set max value
template <DataType v> class Case_Max {
public:
  static inline auto Run() { return 0; }
};

template <> class Case_Max<DataType::CHAR> {
public:
  static inline auto Run() { return CHAR_MAX; }
};

template <> class Case_Max<DataType::SHORT> {
public:
  static inline auto Run() { return SHRT_MAX; }
};

template <> class Case_Max<DataType::INT> {
public:
  static inline auto Run() { return INT_MAX; }
};

template <> class Case_Max<DataType::LONG> {
public:
  static inline auto Run() { return LLONG_MAX; }
};

template <> class Case_Max<DataType::BYTE> {
public:
  static inline auto Run() { return UCHAR_MAX; }
};

template <> class Case_Max<DataType::USHORT> {
public:
  static inline auto Run() { return USHRT_MAX; }
};

template <> class Case_Max<DataType::UINT> {
public:
  static inline auto Run() { return UINT_MAX; }
};

template <> class Case_Max<DataType::ULONG> {
public:
  static inline auto Run() { return ULLONG_MAX; }
};

template <> class Case_Max<DataType::BOOL> {
public:
  static inline auto Run() { return true; }
};

template <> class Case_Max<DataType::DATETIME> {
public:
  static inline auto Run() { return ULLONG_MAX; }
};

template <> class Case_Max<DataType::FLOAT> {
public:
  static inline auto Run() { return FLT_MAX; }
};

template <> class Case_Max<DataType::DOUBLE> {
public:
  static inline auto Run() { return DBL_MAX; }
};

template <class T, DataType DT> inline T MaxValue() {
  return Case_Max<DT>::Run();
}

// sprintf
template <DataType v> class Case_Pri {
public:
  static inline int Sprintf(char *dest, int val) {
    return sprintf(dest, "%d", (int32_t)val);
  }
};

template <> class Case_Pri<DataType::CHAR> {
public:
  static inline int Sprintf(char *dest, int8_t val) {
    return sprintf(dest, "%d", val);
  }
};

template <> class Case_Pri<DataType::SHORT> {
public:
  static inline int Sprintf(char *dest, int16_t val) {
    return sprintf(dest, "%d", val);
  }
};

template <> class Case_Pri<DataType::INT> {
public:
  static inline int Sprintf(char *dest, int32_t val) {
    return sprintf(dest, "%d", val);
  }
};

template <> class Case_Pri<DataType::LONG> {
public:
  static inline int Sprintf(char *dest, int64_t val) {
#ifdef _MSVC_LANG
    return sprintf(dest, "%lld", val);
#else
    return sprintf(dest, "%ld", val);
#endif // _MSVC_LANG
  }
};

template <> class Case_Pri<DataType::BYTE> {
public:
  static inline int Sprintf(char *dest, Byte val) {
    return sprintf(dest, "%u", val);
  }
};

template <> class Case_Pri<DataType::USHORT> {
public:
  static inline int Sprintf(char *dest, uint16_t val) {
    return sprintf(dest, "%u", val);
  }
};

template <> class Case_Pri<DataType::UINT> {
public:
  static inline int Sprintf(char *dest, uint32_t val) {
    return sprintf(dest, "%d", val);
  }
};

template <> class Case_Pri<DataType::ULONG> {
public:
  static inline int Sprintf(char *dest, uint64_t val) {
#ifdef _MSVC_LANG
    return sprintf(dest, "%llu", val);
#else
    return sprintf(dest, "%lu", val);
#endif // _MSVC_LANG
  }
};

template <> class Case_Pri<DataType::BOOL> {
public:
  static inline int Sprintf(char *dest, bool val) {
    return sprintf(dest, val ? "true" : "false");
  }
};

template <> class Case_Pri<DataType::DATETIME> {
public:
  static inline int Sprintf(char *dest, uint64_t val) {
#ifdef _MSVC_LANG
    return sprintf(dest, "%llu", val);
#else
    return sprintf(dest, "%lu", val);
#endif // _MSVC_LANG
  }
};

template <> class Case_Pri<DataType::FLOAT> {
public:
  static inline int Sprintf(char *dest, float val) {
    return sprintf(dest, "%f", val);
  }
};

template <> class Case_Pri<DataType::DOUBLE> {
public:
  static inline int Sprintf(char *dest, double val) {
    return sprintf(dest, "%f", val);
  }
};

template <class T, DataType DT> inline int Sprintf(char *dest, T val) {
  return Case_Pri<DT>::Sprintf(dest, val);
}

// Add
template <DataType v> class Case_Add {
public:
  static inline void Add(int &val, const IDataValue &dv) {}
};

template <> class Case_Add<DataType::CHAR> {
public:
  static inline void Add(char &val, const IDataValue &dv) {
    val += (char)dv.GetLong();
  }
};

template <> class Case_Add<DataType::SHORT> {
public:
  static inline void Add(int16_t &val, const IDataValue &dv) {
    val += (int16_t)dv.GetLong();
  }
};

template <> class Case_Add<DataType::INT> {
public:
  static inline void Add(int32_t &val, const IDataValue &dv) {
    val += (int32_t)dv.GetLong();
  }
};

template <> class Case_Add<DataType::LONG> {
public:
  static inline void Add(int64_t &val, const IDataValue &dv) {
    val += (int64_t)dv.GetLong();
  }
};

template <> class Case_Add<DataType::BYTE> {
public:
  static inline void Add(Byte &val, const IDataValue &dv) {
    val += (Byte)dv.GetLong();
  }
};

template <> class Case_Add<DataType::USHORT> {
public:
  static inline void Add(uint16_t &val, const IDataValue &dv) {
    val += (uint16_t)dv.GetLong();
  }
};

template <> class Case_Add<DataType::UINT> {
public:
  static inline void Add(uint32_t &val, const IDataValue &dv) {
    val += (uint32_t)dv.GetLong();
  }
};

template <> class Case_Add<DataType::ULONG> {
public:
  static inline void Add(uint64_t &val, const IDataValue &dv) {
    val += (uint64_t)dv.GetLong();
  }
};

template <> class Case_Add<DataType::BOOL> {
public:
  static inline void Add(bool &val, const IDataValue &dv) {}
};

template <> class Case_Add<DataType::DATETIME> {
public:
  static inline void Add(uint64_t &val, const IDataValue &dv) {}
};

template <> class Case_Add<DataType::FLOAT> {
public:
  static inline void Add(float &val, const IDataValue &dv) {
    val += (float)dv.GetDouble();
  }
};

template <> class Case_Add<DataType::DOUBLE> {
public:
  static inline void Add(double &val, const IDataValue &dv) {
    val += dv.GetDouble();
  }
};

template <typename T, bool Enabled = std::is_same<T, bool>::value>
class Bool_Add;

template <typename T> struct Bool_Add<T, false> {
  static inline void Add(T &val, const char *pBuf) { val += (T)atoll(pBuf); }
};

template <typename T> struct Bool_Add<T, true> {
  static inline void Add(bool &val, const char *pBuf) { abort(); }
};
} // namespace storage
