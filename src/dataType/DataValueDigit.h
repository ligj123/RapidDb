#pragma once
#include "../config/ErrorID.h"
#include "../utils/BytesConvert.h"
#include "../utils/ErrorMsg.h"
#include "IDataValue.h"
#include "Metadata.h"

namespace storage {
template <class T, DataType DT> class DataValueDigit : public IDataValue {
public:
  DataValueDigit()
      : IDataValue(DT, ValueType::NULL_VALUE, SavePosition::ALL), _value(0) {}
  DataValueDigit(T val)
      : IDataValue(DT, ValueType::SOLE_VALUE, SavePosition::ALL), _value(val) {}
  DataValueDigit(const DataValueDigit &src) : IDataValue(src) {
    _value = src._value;
  }
  ~DataValueDigit() {}

  void PutValue(T val)) {
    _value = val;
  }

  bool Copy(const IDataValue &dv, bool bMove = false) override {
    if (dv.IsNull()) {
      valType_ = ValueType::NULL_VALUE;
      return;
    }
    if (dataType_ == dv.GetDataType()) {
      _value = ((DataValueDigit &)dv)._value;
    } else if (dv.IsStringType()) {
      if (IsAutoPrimaryKey())
        _value = (T)std::atoi(any_cast<MString>(dv.GetValue()).c_str());
      else
        _value = (T)std::atof(any_cast<MString>(dv.GetValue()).c_str());
    } else if (!dv.IsDigital()) {
      _threadErrorMsg = new ErrorMsg(
          2001, {StrOfDataType(dv.GetDataType()), StrOfDataType(dataType_)});
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
  bool Equal(const IDataValue &dv) const override {
    if (dataType_ == dv.GetDataType()) {
      return _value == (T &)dv;
    }
    if (IsAutoPrimaryKey() && dv.IsAutoPrimaryKey()) {
      return (GetLong() == dv.GetLong());
    } else if (dv.IsDigital()) {
      return (GetDouble() == dv.GetDouble());
    } else {
      throw ErrorMsg(DT_UNSUPPORT_CONVERT,
                     {StrOfDataType(dv.GetDataType()), StrOfDataType(DT)});
    }
  }

  uint32_t WriteData(Byte *buf, SavePosition svPos) const override {
    assert(svPos != SavePosition::ALL);
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
  uint32_t ReadData(Byte *buf, uint32_t len, bool bSole = true) override {
    if (bKey_) {
      valType_ = ValueType::SOLE_VALUE;
      _value = DigitalFromBytes<T, DT>(buf, bKey_);
      return sizeof(T);
    } else {
      valType_ = (len != 0 ? ValueType::SOLE_VALUE : ValueType::NULL_VALUE);
      if (valType_ == ValueType::NULL_VALUE)
        return 0;

      _value = DigitalFromBytes<T, DT>(buf, bKey_);
      return sizeof(T);
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
    valType_ = src.valType_;
    bKey_ = src.bKey_;
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
  bool Add(IDataValue &dv) override { Case_Add<DT>::Add(_value, dv); }

  DataValueDigit *operator+(const IDataValue &dv) {
    if (dv.IsDigital()) {
      Case_Add<DT>::Add(_value, dv);
      return this;
    }

    _threadErrorMsg =
        new ErrorMsg(DT_UNSUPPORT_CONVERT,
                     {StrOfDataType(dv.GetDataType()), StrOfDataType(DT)});
    return nullptr;
  }

  DataValueDigit *operator+(const IDataValue &dv) {
    if (dv.IsDigital()) {
      Case_Add<DT>::Add(_value, dv);
      return this;
    }

    return nullptr;
  }

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
