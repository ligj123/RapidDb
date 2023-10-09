#pragma once

#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "DataValueBlob.h"
#include "DataValueDate.h"
#include "DataValueFixChar.h"
#include "DataValueVarChar.h"
#include <any>

namespace storage {
using namespace std;

inline IDataValue *DataValueFactory(DataType type,
                                    uint32_t maxLen = DEFAULT_MAX_VAR_LEN,
                                    any dfVal = any()) {
  IDataValue *pDv = nullptr;
  switch (type) {
  case DataType::CHAR:
    pDv = new DataValueChar();
    break;
  case DataType::SHORT:
    pDv = new DataValueShort();
    break;
  case DataType::INT:
    pDv = new DataValueInt();
    break;
  case DataType::LONG:
    pDv = new DataValueLong();
    break;
  case DataType::BYTE:
    pDv = new DataValueByte();
    break;
  case DataType::USHORT:
    pDv = new DataValueUShort();
    break;
  case DataType::UINT:
    pDv = new DataValueUInt();
    break;
  case DataType::ULONG:
    pDv = new DataValueULong();
    break;
  case DataType::FLOAT:
    pDv = new DataValueFloat();
    break;
  case DataType::DOUBLE:
    pDv = new DataValueDouble();
    break;
  case DataType::DATETIME:
    pDv = new DataValueDate();
    break;
  case DataType::BOOL:
    pDv = new DataValueBool();
    break;
  case DataType::FIXCHAR:
    pDv = new DataValueFixChar(maxLen);
    break;
  case DataType::VARCHAR:
    pDv = new DataValueVarChar(maxLen);
    break;
  case DataType::BLOB:
    pDv = new DataValueBlob(maxLen);
    break;
  default:
    _threadErrorMsg.reset(
        new ErrorMsg(DT_UNKNOWN_TYPE, {ToMString((uint32_t)type)}));
    return nullptr;
  }

  if (dfVal.has_value())
    pDv->PutValue(dfVal);

  return pDv;
}

inline std::ostream &operator<<(std::ostream &os, const IDataValue &dv) {
  switch (dv.dataType_) {
  case DataType::CHAR:
    os << (const DataValueChar &)dv;
    break;
  case DataType::SHORT:
    os << (const DataValueShort &)dv;
    break;
  case DataType::INT:
    os << (const DataValueInt &)dv;
    break;
  case DataType::LONG:
    os << (const DataValueLong &)dv;
    break;
  case DataType::BYTE:
    os << (const DataValueByte &)dv;
    break;
  case DataType::USHORT:
    os << (const DataValueUShort &)dv;
    break;
  case DataType::UINT:
    os << (const DataValueUInt &)dv;
    break;
  case DataType::ULONG:
    os << (const DataValueULong &)dv;
    break;
  case DataType::FLOAT:
    os << (const DataValueFloat &)dv;
    break;
  case DataType::DOUBLE:
    os << (const DataValueDouble &)dv;
    break;
  case DataType::DATETIME:
    os << (const DataValueDateTime &)dv;
    break;
  case DataType::BOOL:
    os << (const DataValueBool &)dv;
    break;
  case DataType::FIXCHAR:
    os << (const DataValueFixChar &)dv;
    break;
  case DataType::VARCHAR:
    os << (const DataValueVarChar &)dv;
    break;
  case DataType::BLOB:
    os << (const DataValueBlob &)dv;
    break;
  default:
    abort();
  }

  return os;
}

inline bool operator==(const IDataValue &dv1, const IDataValue &dv2) {
  if (dv1.GetDataType() == dv2.GetDataType()) {
    return dv1.EQ(dv2);
  }
  if (dv1.IsAutoPrimaryKey() && dv2.IsAutoPrimaryKey()) {
    int64_t l1 = dv1.GetLong();
    int64_t l2 = dv2.GetLong();
    return (l1 == l2);
  }
  if (dv1.IsDigital() && dv2.IsDigital()) {
    double d1 = dv1.GetDouble();
    double d2 = dv2.GetDouble();
    return (d1 == d2);
  }

  if (dv1.IsStringType() && dv2.IsStringType()) {
    return (strcmp((char *)dv1.GetBuff(), (char *)dv2.GetBuff()) == 0);
  }

  abort();
}

inline bool operator>(const IDataValue &dv1, const IDataValue &dv2) {
  if (dv1.GetDataType() == dv2.GetDataType()) {
    return dv1.GT(dv2);
  }

  if (dv1.IsAutoPrimaryKey() && dv2.IsAutoPrimaryKey()) {
    int64_t l1 = dv1.GetLong();
    int64_t l2 = dv2.GetLong();
    return (l1 > l2);
  }
  if (dv1.IsDigital() && dv2.IsDigital()) {
    double d1 = dv1.GetDouble();
    double d2 = dv2.GetDouble();
    return (d1 > d2);
  }

  if (dv1.IsArrayType() && dv2.IsArrayType()) {
    return (strcmp((char *)dv1.GetBuff(), (char *)dv2.GetBuff()) > 0);
  }

  abort();
}

inline bool operator>=(const IDataValue &dv1, const IDataValue &dv2) {
  if (dv1.GetDataType() == dv2.GetDataType()) {
    return !dv1.LT(dv2);
  }

  if (dv1.IsAutoPrimaryKey() && dv2.IsAutoPrimaryKey()) {
    int64_t l1 = dv1.GetLong();
    int64_t l2 = dv2.GetLong();
    return (l1 >= l2);
  }
  if (dv1.IsDigital() && dv2.IsDigital()) {
    double d1 = dv1.GetDouble();
    double d2 = dv2.GetDouble();
    return (d1 >= d2);
  }

  if (dv1.IsArrayType() && dv2.IsArrayType()) {
    return (strcmp((char *)dv1.GetBuff(), (char *)dv2.GetBuff()) >= 0);
  }

  abort();
}

inline bool operator!=(const IDataValue &dv1, const IDataValue &dv2) {
  return !(dv1 == dv2);
}

inline bool operator<(const IDataValue &dv1, const IDataValue &dv2) {
  return !(dv1 >= dv2);
}

inline bool operator<=(const IDataValue &dv1, const IDataValue &dv2) {
  return !(dv1 > dv2);
}
} // namespace storage
