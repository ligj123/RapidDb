#pragma once
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include "IDataValue.h"
#include "Metadata.h"
#include <memory>

namespace storage {
template <class T, DataType DT> class DataValueDigit : public IDataValue {
public:
  DataValueDigit() : IDataValue(DT, ValueType::NULL_VALUE), _value(0) {}
  DataValueDigit(T val) : IDataValue(DT, ValueType::SOLE_VALUE), _value(val) {}
  DataValueDigit(const DataValueDigit &src) : IDataValue(src) {
    _value = src._value;
  }

  bool SetValue(T val) {
    _value = val;
    valType_ = ValueType::SOLE_VALUE;
    return true;
  }

  void SetNull() override { valType_ = ValueType::NULL_VALUE; }

  bool PutValue(std::any val) override {
    if (val.type() == typeid(int64_t))
      _value = (T)std::any_cast<int64_t>(val);
    else if (val.type() == typeid(int32_t))
      _value = (T)std::any_cast<int32_t>(val);
    else if (val.type() == typeid(int16_t))
      _value = (T)std::any_cast<int16_t>(val);
    else if (val.type() == typeid(uint64_t))
      _value = (T)std::any_cast<uint64_t>(val);
    else if (val.type() == typeid(uint32_t))
      _value = (T)std::any_cast<uint32_t>(val);
    else if (val.type() == typeid(uint16_t))
      _value = (T)std::any_cast<uint16_t>(val);
    else if (val.type() == typeid(int8_t))
      _value = std::any_cast<int8_t>(val);
    else if (val.type() == typeid(uint8_t))
      _value = std::any_cast<uint8_t>(val);
    else if (val.type() == typeid(double))
      _value = (T)any_cast<double>(val);
    else if (val.type() == typeid(float))
      _value = (T)any_cast<float>(val);
    else if (val.type() == typeid(MString)) {
      if (IDataValue::IsAutoPrimaryKey(DT))
        _value = (T)stoi(std::any_cast<MString>(val).c_str());
      else
        _value = (T)stod(std::any_cast<MString>(val).c_str());
    } else if (val.type() == typeid(string)) {
      if (IDataValue::IsAutoPrimaryKey(DT))
        _value = (T)stoi(std::any_cast<string>(val));
      else
        _value = (T)stod(std::any_cast<string>(val));
    } else {
      abort();
    }

    valType_ = ValueType::SOLE_VALUE;
    return true;
  }

  bool Copy(const IDataValue &dv, bool bMove = false) override {
    if (dv.IsNull()) {
      valType_ = ValueType::NULL_VALUE;
      return true;
    }
    if (dataType_ == dv.GetDataType()) {
      _value = ((DataValueDigit &)dv)._value;
    } else if (dv.IsStringType()) {
      if (IsAutoPrimaryKey())
        _value = (T)std::atoi(any_cast<MString>(dv.GetValue()).c_str());
      else
        _value = (T)std::atof(any_cast<MString>(dv.GetValue()).c_str());
    } else if (!dv.IsDigital()) {
      _threadErrorMsg.reset(new ErrorMsg(
          2001, {StrOfDataType(dv.GetDataType()), StrOfDataType(dataType_)}));
      return false;
    } else if (IsAutoPrimaryKey()) {
      _value = (T)dv.GetLong();
    } else {
      _value = (T)dv.GetDouble();
    }

    valType_ = ValueType::SOLE_VALUE;
    return true;
  }

  DataValueDigit *Clone(bool incVal = false) override {
    return new DataValueDigit(*this);
  }
  uint32_t GetPersistenceLength(SavePosition dtPos) const override {
    return dtPos == SavePosition::KEY
               ? sizeof(T)
               : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(T));
  };
  uint32_t GetDataLength() const override {
    return valType_ == ValueType::NULL_VALUE ? 0 : sizeof(T);
  }
  uint32_t GetMaxLength() const override { return sizeof(T); }
  std::any GetValue() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return std::any();
    else
      return _value;
  }
  int64_t GetLong() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return (int64_t)_value;
  }
  double GetDouble() const override {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return (double)_value;
  }
  size_t Hash() const override { return std::hash<T>{}(_value); }

  uint32_t WriteData(Byte *buf, SavePosition svPos) const override {
    assert(svPos != SavePosition::UNKNOWN);
    if (svPos == SavePosition::KEY) {
      if (valType_ == ValueType::NULL_VALUE) {
        DigitalToBytes<T, DT>(0, buf, true);
      } else {
        DigitalToBytes<T, DT>(_value, buf, true);
      }
      return sizeof(T);
    } else {
      if (valType_ == ValueType::NULL_VALUE) {
        return 0;
      } else {
        DigitalToBytes<T, DT>(_value, buf, false);
        return sizeof(T);
      }
    }
  }
  uint32_t ReadData(Byte *buf, uint32_t len, SavePosition svPos,
                    bool bSole = true) override {
    assert(svPos != SavePosition::UNKNOWN);
    if (svPos == SavePosition::KEY) {
      valType_ = ValueType::SOLE_VALUE;
      _value = DigitalFromBytes<T, DT>(buf, true);
      return sizeof(T);
    } else {
      valType_ = (len != 0 ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
      if (valType_ == ValueType::NULL_VALUE)
        return 0;

      _value = DigitalFromBytes<T, DT>(buf, false);
      return sizeof(T);
    }
  }
  uint32_t WriteData(Byte *buf) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      buf[0] = (Byte)dataType_ & DATE_TYPE;
      return 1;
    } else {
      buf[0] = VALUE_TYPE | ((Byte)dataType_ & DATE_TYPE);
      DigitalToBytes<T, DT>(_value, buf + 1, false);
      return sizeof(T) + 1;
    }
  }
  uint32_t ReadData(Byte *buf) override {
    valType_ =
        ((buf[0] & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
    if (valType_ == ValueType::NULL_VALUE)
      return 1;

    _value = DigitalFromBytes<T, DT>(buf + 1, false);
    return sizeof(T) + 1;
  }
  void SetMinValue() override {
    valType_ = ValueType::SOLE_VALUE;
    _value = MinValue<T, DT>();
  }
  void SetMaxValue() override {
    valType_ = ValueType::SOLE_VALUE;
    _value = MaxValue<T, DT>();
  }
  void SetDefaultValue() override {
    _value = 0;
    valType_ = ValueType::SOLE_VALUE;
  }

  operator T() const {
    if (valType_ == ValueType::NULL_VALUE)
      return 0;
    else
      return _value;
  }
  DataValueDigit &operator=(T val) {
    valType_ = ValueType::SOLE_VALUE;
    _value = val;
    return *this;
  }
  DataValueDigit &operator=(const DataValueDigit &src) {
    valType_ = src.valType_;
    dataType_ = src.dataType_;
    _value = src._value;

    return *this;
  }

  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }
    if (24 > sb.GetFreeLen()) {
      sb.Resize(sb.GetStrLen() + 24);
    }

    char *dest = sb.GetFreeBuff();
    int n = Sprintf<T, DT>(dest, _value);
    sb.SetStrLen(sb.GetStrLen() + n);
  }

  bool EQ(const IDataValue &dv) const override {
    assert(dv.IsDigital());
    if (dataType_ == dv.GetDataType()) {
      return *this == (DataValueDigit &)dv;
    }

    if (IsNull()) {
      return dv.IsNull();
    } else if (dv.IsNull()) {
      return false;
    } else if (IsAutoPrimaryKey() && dv.IsAutoPrimaryKey()) {
      return (GetLong() == dv.GetLong());
    } else if (dv.IsDigital()) {
      return (GetDouble() == dv.GetDouble());
    }

    return false;
  }

  bool GT(const IDataValue &dv) const override {
    assert(dv.IsDigital());
    if (dataType_ == dv.GetDataType()) {
      return *this > (DataValueDigit &)dv;
    }

    if (IsNull()) {
      return false;
    } else if (dv.IsNull()) {
      return true;
    } else if (IsAutoPrimaryKey() && dv.IsAutoPrimaryKey()) {
      return (GetLong() > dv.GetLong());
    } else if (dv.IsDigital()) {
      return (GetDouble() > dv.GetDouble());
    }

    return false;
  }

  bool LT(const IDataValue &dv) const override {
    assert(dv.IsDigital());
    if (dataType_ == dv.GetDataType()) {
      return *this < (DataValueDigit &)dv;
    }

    if (dv.IsNull()) {
      return false;
    } else if (IsNull()) {
      return true;
    } else if (IsAutoPrimaryKey() && dv.IsAutoPrimaryKey()) {
      return (GetLong() < dv.GetLong());
    } else if (dv.IsDigital()) {
      return (GetDouble() < dv.GetDouble());
    }

    return false;
  }

  bool operator>(const DataValueDigit &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return false;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }

    return _value > dv._value;
  }
  bool operator<(const DataValueDigit &dv) const { return !(*this >= dv); }
  bool operator>=(const DataValueDigit &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return true;
    }
    return _value >= dv._value;
  }
  bool operator<=(const DataValueDigit &dv) const { return !(*this > dv); }
  bool operator==(const DataValueDigit &dv) const {
    if (valType_ == ValueType::NULL_VALUE) {
      return dv.valType_ == ValueType::NULL_VALUE;
    }
    if (dv.valType_ == ValueType::NULL_VALUE) {
      return false;
    }
    return _value == dv._value;
  }
  bool operator!=(const DataValueDigit &dv) const { return !(*this == dv); }
  bool Add(IDataValue &dv) override {
    if (dv.IsDigital()) {
      Case_Add<DT>::Add(_value, dv);
    } else if (dv.IsStringType()) {
      Bool_Add<T>::Add(_value, (char *)dv.GetBuff());
    } else {
      _threadErrorMsg.reset(
          new ErrorMsg(DT_UNSUPPORT_CONVERT,
                       {StrOfDataType(dv.GetDataType()), StrOfDataType(DT)}));
      return false;
    }
    return true;
  }

  template <class V, DataType DTV>
  DataValueDigit *operator+(const DataValueDigit<V, DTV> &dv) {
    _value += (T)(V)dv;
    return this;
  }
  template <class V, DataType DTV>
  DataValueDigit *operator-(const DataValueDigit<V, DTV> &dv) {
    _value -= (T)(V)dv;
    return this;
  }
  template <class V, DataType DTV>
  DataValueDigit *operator*(const DataValueDigit<V, DTV> &dv) {
    _value *= (T)(V)dv;
    return this;
  }
  template <class V, DataType DTV>
  DataValueDigit *operator/(const DataValueDigit<V, DTV> &dv) {
    if ((V)dv == 0) {
      _threadErrorMsg.reset(new ErrorMsg(DT_UNSUPPORT_OPER, {"N/0", "ALL"}));
      return nullptr;
    }

    _value /= (T)(V)dv;
    return this;
  }

protected:
  ~DataValueDigit() {}

protected:
  T _value;
};

template <class T, DataType DT>
inline std::ostream &operator<<(std::ostream &os,
                                const DataValueDigit<T, DT> &dv) {
  if (dv.GetValueType() == ValueType::NULL_VALUE)
    os << "nullptr";
  else
    os << (T)dv;

  return os;
}

typedef DataValueDigit<char, DataType::CHAR> DataValueChar;
typedef DataValueDigit<Byte, DataType::BYTE> DataValueByte;
typedef DataValueDigit<bool, DataType::BOOL> DataValueBool;
typedef DataValueDigit<int16_t, DataType::SHORT> DataValueShort;
typedef DataValueDigit<uint16_t, DataType::USHORT> DataValueUShort;
typedef DataValueDigit<int32_t, DataType::INT> DataValueInt;
typedef DataValueDigit<uint32_t, DataType::UINT> DataValueUInt;
typedef DataValueDigit<int64_t, DataType::LONG> DataValueLong;
typedef DataValueDigit<uint64_t, DataType::ULONG> DataValueULong;
typedef DataValueDigit<float, DataType::FLOAT> DataValueFloat;
typedef DataValueDigit<double, DataType::DOUBLE> DataValueDouble;
typedef DataValueDigit<uint64_t, DataType::DATETIME> DataValueDate;
} // namespace storage
