#pragma once
#include "DataValueDigit.h"

namespace storage {
class DataValueDateTime : public DataValueDigit<uint64_t, DataType::DATETIME> {
public:
  DataValueDateTime() : DataValueDigit() {}
  DataValueDateTime(uint64_t millSec/*The milliseconds since epoch*/)
      : DataValueDigit(millSec) {}
  DataValueDateTime(char* dtStr, size_t dtLen, const char* format) : DataValueDigit() {
    
  }
  DataValueDate(const DataValueDate &src) : DataValueDigit(src) {}
  ~DataValueDate() {}

public:
  DataValueDate *Clone(bool incVal = false) override {
    return new DataValueDate(*this);
  }
  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }
    if (22 > sb.GetFreeLen()) {
      sb.Resize(sb.GetStrLen() + 22);
    }

    char *dest = sb.GetFreeBuff();
    int n = sprintf(dest, "%llu", _value);
    sb.SetStrLen(sb.GetStrLen() + n);
  }

protected:
static MString _dtFormat;
  friend std::ostream &operator<<(std::ostream &os, const DataValueDate &dv);
};

inline std::ostream &operator<<(std::ostream &os, const DataValueDate &dv) {
  if (dv.valType_ == ValueType::NULL_VALUE)
    os << "nullptr";
  else
    os << dv._value;

  return os;
}
} // namespace storage
