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

namespace storage {
  using namespace std;
  class DataValueFactory
  {
  public:
    static IDataValue* GenerateDataValue(DataType type, bool bKey = false, uint32_t maxLen = -1, any dfVal = any())
    {
      IDataValue* pDv = nullptr;
      switch (type)
      {
      case DataType::CHAR:
        pDv = new DataValueChar(dfVal, bKey);
        break;
      case DataType::SHORT:
        pDv = new DataValueShort(dfVal, bKey);
        break;
      case DataType::INT:
        pDv = new DataValueInt(dfVal, bKey);
        break;
      case DataType::LONG:
        pDv = new DataValueLong(dfVal, bKey);
        break;
      case DataType::BYTE:
        pDv = new DataValueByte(dfVal, bKey);
        break;
      case DataType::USHORT:
        pDv = new DataValueUShort(dfVal, bKey);
        break;
      case DataType::UINT:
        pDv = new DataValueUInt(dfVal, bKey);
        break;
      case DataType::ULONG:
        pDv = new DataValueULong(dfVal, bKey);
        break;
      case DataType::FLOAT:
        pDv = new DataValueFloat(dfVal, bKey);
        break;
      case DataType::DOUBLE:
        pDv = new DataValueDouble(dfVal, bKey);
        break;
      case DataType::DATETIME:
        pDv = new DataValueDate(dfVal, bKey);
        break;
      case DataType::BOOL:
        pDv = new DataValueBool(dfVal, bKey);
        break;
      case DataType::FIXCHAR:
        pDv = new DataValueFixChar(maxLen, bKey, dfVal);
        break;
      case DataType::VARCHAR:
        pDv = new DataValueVarChar(maxLen, bKey, dfVal);
        break;
      case DataType::BLOB:
        pDv = new DataValueBlob(maxLen, bKey);
        break;
      default:
        throw new ErrorMsg(DT_UNKNOWN_TYPE, {to_string((uint32_t)type)});
      }

      return pDv;
    }
  };
}

