#pragma once
#include "DataValueDigit.h"
#include <any>
#include <ostream>

namespace storage {
// class DataValueDate : public DataValueDigit<uint64_t, DataType::DATETIME> {
// public:
//  DataValueDate(bool bKey = false) : DataValueDigit(bKey) {}
//  DataValueDate(uint64_t msVal, bool bKey = false)
//      : DataValueDigit(msVal, bKey) {}
//  DataValueDate(std::any val, bool bKey = false) : DataValueDigit(val, bKey)
//  {} DataValueDate(const DataValueDate &src) : DataValueDigit(src) {}
//  ~DataValueDate() {}
//
// public:
//  DataValueDate *Clone(bool incVal = false) override {
//    return new DataValueDate(*this);
//  }
//  void ToString(StrBuff &sb) const override {
//    if (valType_ == ValueType::NULL_VALUE) {
//      return;
//    }
//    if (22 > sb.GetFreeLen()) {
//      sb.Resize(sb.GetStrLen() + 22);
//    }
//
//    char *dest = sb.GetFreeBuff();
//    int n = sprintf(dest, "%llu", _value);
//    sb.SetStrLen(sb.GetStrLen() + n);
//  }
//
//  friend std::ostream &operator<<(std::ostream &os, const DataValueDate &dv);
//};
//
// inline std::ostream &operator<<(std::ostream &os, const DataValueDate &dv) {
//  if (dv.valType_ == ValueType::NULL_VALUE)
//    os << "nullptr";
//  else
//    os << dv._value;
//
//  return os;
//}
} // namespace storage
