#pragma once
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include "IDataValue.h"
#include "Metadata.h"

namespace storage {
template <class T, DataType DT> class DataValueDigit : public IDataValue {
public:
  DataValueDigit(bool key = false)
      : IDataValue(DT, ValueType::NULL_VALUE, key), _value(0) {}
  DataValueDigit(T val, bool key = false)
      : IDataValue(DT, ValueType::SOLE_VALUE, key), _value(val) {}
  DataValueDigit(std::any val, bool bKey = false)
      : IDataValue(DT, ValueType::SOLE_VALUE, bKey) {
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
    else if (val.type() == typeid(std::string)) {
      if (IDataValue::IsAutoPrimaryKey(DT))
        _value = (T)std::stoi(std::any_cast<std::string>(val));
      else
        _value = (T)std::stod(std::any_cast<std::string>(val));
    } else
      throw utils::ErrorMsg(2001, {val.type().name(), StrOfDataType(DT)});
  }

  DataValueDigit(const DataValueDigit &src) : IDataValue(src) {
    _value = src._value;
  }
  ~DataValueDigit() {}

  void Copy(IDataValue &dv, bool bMove = false) override {
    if (dv.IsNull()) {
      valType_ = ValueType::NULL_VALUE;
      return;
    }
    if (dataType_ == dv.GetDataType()) {
      _value = ((DataValueDigit &)dv)._value;
    } else if (dv.IsStringType()) {
      if (IsAutoPrimaryKey())
        _value = (T)std::atoi(any_cast<string>(dv.GetValue()).c_str());
      else
        _value = (T)std::atof(any_cast<string>(dv.GetValue()).c_str());
    } else if (!dv.IsDigital()) {
      throw utils::ErrorMsg(
          2001, {StrOfDataType(dv.GetDataType()), StrOfDataType(dataType_)});
    } else if (IsAutoPrimaryKey()) {
      _value = (T)dv.GetLong();
    } else {
      _value = (T)dv.GetDouble();
    }

    valType_ = ValueType::SOLE_VALUE;
  }

  DataValueDigit *Clone(bool incVal = false) override {
    return new DataValueDigit(*this);
  }

  uint32_t GetPersistenceLength() const override {
    return GetPersistenceLength(bKey_);
  }
  uint32_t GetPersistenceLength(bool key) const override {
    return key ? sizeof(T)
               : (valType_ == ValueType::NULL_VALUE ? 1 : 1 + sizeof(T));
  };
  uint32_t GetDataLength() const override {
    return bKey_ ? sizeof(T)
                 : (valType_ == ValueType::NULL_VALUE ? 0 : sizeof(T));
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
  size_t Hash() const override { return std::hash<T>{}((T) * this); }
  bool Equal(const IDataValue &dv) const override {
    if (IsAutoPrimaryKey() && dv.IsAutoPrimaryKey()) {
      return (GetLong() == dv.GetLong());
    } else {
      return (GetDouble() == dv.GetDouble());
    }
  }

  uint32_t WriteData(Byte *buf) const override {
    return WriteData(buf, bKey_);
  };

  uint32_t WriteData(Byte *buf, bool key) const override {
    if (key) {
      if (valType_ == ValueType::SOLE_VALUE) {
        utils::DigitalToBytes<T>(_value, buf, true);
      }
      return sizeof(T);
    } else {
      if (valType_ == ValueType::NULL_VALUE) {
        *buf = ((Byte)dataType_ & DATE_TYPE);
        return 1;
      } else {
        *buf = (VALUE_TYPE | ((Byte)dataType_ & DATE_TYPE));
        buf++;
        DigitalToBytes<T, DT>(_value, buf, false);
        return sizeof(T) + 1;
      }
    }
  }
  uint32_t ReadData(Byte *buf, uint32_t len = 0, bool bSole = true) override {
    if (bKey_) {
      valType_ = ValueType::SOLE_VALUE;
      _value = utils::DigitalFromBytes<T>(buf, bKey_);
      return sizeof(T);
    } else {
      valType_ =
          (*buf & VALUE_TYPE ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
      buf++;

      if (valType_ == ValueType::NULL_VALUE)
        return 1;

      _value = DigitalFromBytes<T, DT>(buf, bKey_);
      return sizeof(T) + 1;
    }
  }
  uint32_t WriteData(fstream &fs) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      fs.put((Byte)dataType_ & DATE_TYPE);
      return 1;
    } else {
      fs.put(VALUE_TYPE | ((Byte)dataType_ & DATE_TYPE));
      fs.write((char *)&_value, sizeof(T));
      return sizeof(T) + 1;
    }
  }
  uint32_t ReadData(fstream &fs) override {
    Byte by;
    fs.read((char *)&by, 1);
    valType_ =
        ((by & VALUE_TYPE) ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
    if (valType_ == ValueType::NULL_VALUE)
      return 1;

    fs.read((char *)&_value, sizeof(uint64_t));
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
    dataType_ = src.dataType_;
    valType_ = src.valType_;
    bKey_ = src.bKey_;
    _value = src._value;

    return *this;
  }

  void ToString(StrBuff &sb) const override {
    if (valType_ == ValueType::NULL_VALUE) {
      return;
    }
    if (22 > sb.GetFreeLen()) {
      sb.Resize(sb.GetStrLen() + 22);
    }

    char *dest = sb.GetFreeBuff();
    int n = 0;
    switch (sizeof(T)) {
    case 1:
      if (typeid(T) == typeid(char))
        n = sprintf(dest, "%d", (char)_value);
      else if (typeid(T) == typeid(bool))
        n = sprintf(dest, _value ? "true" : "false");
      else
        n = sprintf(dest, "%u", (Byte)_value);
      break;
    case 2:
      if (typeid(T) == typeid(int16_t))
        n = sprintf(dest, "%d", (int16_t)_value);
      else
        n = sprintf(dest, "%u", (uint16_t)_value);
      break;
    case 4:
      if (typeid(T) == typeid(int32_t))
        n = sprintf(dest, "%d", (int32_t)_value);
      else if (typeid(T) == typeid(uint32_t))
        n = sprintf(dest, "%u", (uint32_t)_value);
      else
        n = sprintf(dest, "%f", (float)_value);
      break;
    case 8:
      if (typeid(T) == typeid(int64_t))
        n = sprintf(dest, "%lld", (int64_t)_value);
      else if (typeid(T) == typeid(uint64_t))
        n = sprintf(dest, "%llu", (uint64_t)_value);
      else
        n = sprintf(dest, "%f", (double)_value);
      break;
    }

    sb.SetStrLen(sb.GetStrLen() + n);
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
  // friend std::ostream &operator<<(std::ostream &os,
  //                                const DataValueDigit<T, DT> &dv);

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
