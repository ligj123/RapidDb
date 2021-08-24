#pragma once
#include "../utils/BytesConvert.h"
#include "DataType.h"
namespace storage {
// From digital to bytes
template <DataType v> class Case_T {
public:
  static inline auto Run() { return nullptr; }
};

template <> class Case_T<DataType::CHAR> {
public:
  static inline auto Run() { return utils::Int8ToBytes; }
};

template <> class Case_T<DataType::SHORT> {
public:
  static inline auto Run() { return utils::Int16ToBytes; }
};

template <> class Case_T<DataType::INT> {
public:
  static inline auto Run() { return utils::Int32ToBytes; }
};

template <> class Case_T<DataType::LONG> {
public:
  static inline auto Run() { return utils::Int64ToBytes; }
};

template <> class Case_T<DataType::BYTE> {
public:
  static inline auto Run() { return utils::UInt8ToBytes; }
};

template <> class Case_T<DataType ::USHORT> {
public:
  static inline auto Run() { return utils::UInt16ToBytes; }
};

template <> class Case_T<DataType::UINT> {
public:
  static inline auto Run() { return utils::UInt32ToBytes; }
};

template <> class Case_T<DataType::ULONG> {
public:
  static inline auto Run() { return utils::UInt64ToBytes; }
};

template <> class Case_T<DataType::BOOL> {
public:
  static inline auto Run() {
    return [](bool val, Byte *pArr, bool bKey) { *pArr = val; };
  }
};

template <> class Case_T<DataType::DATETIME> {
public:
  static inline auto Run() { return utils::UInt64ToBytes; }
};

template <> class Case_T<DataType::FLOAT> {
public:
  static inline auto Run() { return utils::FloatToBytes; }
};

template <> class Case_T<DataType::DOUBLE> {
public:
  static inline auto Run() { return utils::DoubleToBytes; }
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
  static inline auto Run() { return utils::Int8FromBytes; }
};

template <> class Case_F<DataType::SHORT> {
public:
  static inline auto Run() { return utils::Int16FromBytes; }
};

template <> class Case_F<DataType::INT> {
public:
  static inline auto Run() { return utils::Int32FromBytes; }
};

template <> class Case_F<DataType::LONG> {
public:
  static inline auto Run() { return utils::Int64FromBytes; }
};

template <> class Case_F<DataType::BYTE> {
public:
  static inline auto Run() { return utils::UInt8FromBytes; }
};

template <> class Case_F<DataType::USHORT> {
public:
  static inline auto Run() { return utils::UInt16FromBytes; }
};

template <> class Case_F<DataType::UINT> {
public:
  static inline auto Run() { return utils::UInt32FromBytes; }
};

template <> class Case_F<DataType::ULONG> {
public:
  static inline auto Run() { return utils::UInt64FromBytes; }
};

template <> class Case_F<DataType::BOOL> {
public:
  static inline auto Run() {
    return [](Byte *pArr, bool bKey) { return *pArr; };
  }
};

template <> class Case_F<DataType::DATETIME> {
public:
  static inline auto Run() { return utils::UInt64FromBytes; }
};

template <> class Case_F<DataType::FLOAT> {
public:
  static inline auto Run() { return utils::FloatFromBytes; }
};

template <> class Case_F<DataType::DOUBLE> {
public:
  static inline auto Run() { return utils::DoubleFromBytes; }
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
} // namespace storage
