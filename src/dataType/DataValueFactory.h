#pragma once
#include <any>
#include "IDataValue.h"
#include "DataValueLong.h"
#include "DataValueInt.h"
#include "DataValueShort.h"
#include "DataValueByte.h"

#include "DataValueChar.h"
#include "DataValueUInt.h"
#include "DataValueULong.h"
#include "DataValueUShort.h"

#include "DataValueFloat.h"
#include "DataValueDouble.h"
#include "DataValueBool.h"
#include "DataValueDate.h"

#include "DataValueFixChar.h"
#include "DataValueVarChar.h"
#include "DataValueBlob.h"
#include "../config/ErrorID.h"
#include "../utils/ErrorMsg.h"

namespace storage {
  using namespace std;
 
  inline IDataValue* DataValueFactory(DataType type, bool bKey = false, uint32_t maxLen = -1, any dfVal = any())
  {
    IDataValue* pDv = nullptr;
    switch (type)
    {
    case DataType::CHAR:
      pDv = dfVal.has_value() ? new DataValueChar(dfVal, bKey) : new DataValueChar(bKey);
      break;
    case DataType::SHORT:
      pDv = dfVal.has_value() ? new DataValueShort(dfVal, bKey) : new DataValueShort(bKey);
      break;
    case DataType::INT:
      pDv = dfVal.has_value() ? new DataValueInt(dfVal, bKey) : new DataValueInt(bKey);
      break;
    case DataType::LONG:
      pDv = dfVal.has_value() ? new DataValueLong(dfVal, bKey) : new DataValueLong(bKey);
      break;
    case DataType::BYTE:
      pDv = dfVal.has_value() ? new DataValueByte(dfVal, bKey) : new DataValueByte(bKey);
      break;
    case DataType::USHORT:
      pDv = dfVal.has_value() ? new DataValueUShort(dfVal, bKey) : new DataValueUShort(bKey);
      break;
    case DataType::UINT:
      pDv = dfVal.has_value() ? new DataValueUInt(dfVal, bKey) : new DataValueUInt(bKey);
      break;
    case DataType::ULONG:
      pDv = dfVal.has_value() ? new DataValueULong(dfVal, bKey) : new DataValueULong(bKey);
      break;
    case DataType::FLOAT:
      pDv = dfVal.has_value() ? new DataValueFloat(dfVal, bKey) : new DataValueFloat(bKey);
      break;
    case DataType::DOUBLE:
      pDv = dfVal.has_value() ? new DataValueDouble(dfVal, bKey) : new DataValueDouble(bKey);
      break;
    case DataType::DATETIME:
      pDv = dfVal.has_value() ? new DataValueDate(dfVal, bKey) : new DataValueDate(bKey);
      break;
    case DataType::BOOL:
      pDv = dfVal.has_value() ? new DataValueBool(dfVal, bKey) : new DataValueBool(bKey);
      break;
    case DataType::FIXCHAR:
      pDv = dfVal.has_value() ? new DataValueFixChar(maxLen, bKey, dfVal) : new DataValueFixChar(maxLen, bKey);
      break;
    case DataType::VARCHAR:
      pDv = dfVal.has_value() ? new DataValueVarChar(maxLen, bKey, dfVal) : new DataValueVarChar(maxLen, bKey);
      break;
    case DataType::BLOB:
      pDv = new DataValueBlob(maxLen, bKey);
      break;
    default:
      throw new ErrorMsg(DT_UNKNOWN_TYPE, { to_string((uint32_t)type) });
    }

    return pDv;
  }

  inline std::ostream& operator<< (std::ostream& os, const IDataValue& dv) {
    switch (dv.dataType_)
    {
    case DataType::CHAR:
      os << (const DataValueChar&)dv;
      break;
    case DataType::SHORT:
      os << (const DataValueShort&)dv;
      break;
    case DataType::INT:
      os << (const DataValueInt&)dv;
      break;
    case DataType::LONG:
      os << (const DataValueLong&)dv;
      break;
    case DataType::BYTE:
      os << (const DataValueByte&)dv;
      break;
    case DataType::USHORT:
      os << (const DataValueUShort&)dv;
      break;
    case DataType::UINT:
      os << (const DataValueUInt&)dv;
      break;
    case DataType::ULONG:
      os << (const DataValueULong&)dv;
      break;
    case DataType::FLOAT:
      os << (const DataValueFloat&)dv;
      break;
    case DataType::DOUBLE:
      os << (const DataValueDouble&)dv;
      break;
    case DataType::DATETIME:
      os << (const DataValueDate&)dv;
      break;
    case DataType::BOOL:
      os << (const DataValueBool&)dv;
      break;
    case DataType::FIXCHAR:
      os << (const DataValueFixChar&)dv;
      break;
    case DataType::VARCHAR:
      os << (const DataValueVarChar&)dv;
      break;
    case DataType::BLOB:
      os << (const DataValueBlob&)dv;
      break;
    default:
      throw new ErrorMsg(DT_UNKNOWN_TYPE, { to_string((uint32_t)dv.dataType_) });
    }

    return os;
  }

  inline bool operator== (const IDataValue& dv1, const IDataValue& dv2) {
    assert(dv1.GetDataType() == dv2.GetDataType());
    switch (dv1.dataType_)
    {
    case DataType::CHAR:
      return (const DataValueChar&)dv1 == (const DataValueChar&)dv2;
      break;
    case DataType::SHORT:
      return (const DataValueShort&)dv1 == (const DataValueShort&)dv2;
      break;
    case DataType::INT:
      return (const DataValueInt&)dv1 == (const DataValueInt&)dv2;
      break;
    case DataType::LONG:
      return (const DataValueLong&)dv1 == (const DataValueLong&)dv2;
      break;
    case DataType::BYTE:
      return (const DataValueByte&)dv1 == (const DataValueByte&)dv2;
      break;
    case DataType::USHORT:
      return (const DataValueUShort&)dv1 == (const DataValueUShort&)dv2;
      break;
    case DataType::UINT:
      return (const DataValueUInt&)dv1 == (const DataValueUInt&)dv2;
      break;
    case DataType::ULONG:
      return (const DataValueULong&)dv1 == (const DataValueULong&)dv2;
      break;
    case DataType::FLOAT:
      return (const DataValueFloat&)dv1 == (const DataValueFloat&)dv2;
      break;
    case DataType::DOUBLE:
      return (const DataValueDouble&)dv1 == (const DataValueDouble&)dv2;
      break;
    case DataType::DATETIME:
      return (const DataValueDate&)dv1 == (const DataValueDate&)dv2;
      break;
    case DataType::BOOL:
      return (const DataValueBool&)dv1 == (const DataValueBool&)dv2;
      break;
    case DataType::FIXCHAR:
      return (const DataValueFixChar&)dv1 == (const DataValueFixChar&)dv2;
      break;
    case DataType::VARCHAR:
      return (const DataValueVarChar&)dv1 == (const DataValueVarChar&)dv2;
      break;
    default:
      throw new ErrorMsg(DT_UNKNOWN_TYPE, { to_string((uint32_t)dv1.dataType_) });
    }
  }
}

