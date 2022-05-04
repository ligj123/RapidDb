#pragma once
#include "../config/ErrorID.h"
#include "../core/ActionType.h"
#include "../dataType/DataType.h"
#include "../dataType/DataValueBlob.h"
#include "../dataType/DataValueDigit.h"
#include "../dataType/DataValueFixChar.h"
#include "../dataType/DataValueVarChar.h"
#include "../dataType/IDataValue.h"
#include "../header.h"
#include "../resultset/IResultSet.h"
#include "../transaction/TranType.h"
#include "../transaction/Transaction.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include <chrono>
#include <future>
#include <string>

namespace storage {
class Transaction;
class Statement {
public:
  /**
   * Constuctor for statement
   * @param tran The transaction own this statement, maybe null and create it
   * automately
   * @param vct Used to save the parameters types, maxlength etc.
   * @param statTime If save create, execute and stop time for statistics
   */
  Statement(Transaction *tran, const VectorDataValue *vct, bool statTime)
      : _tran(tran), _bAutoTran(tran == nullptr), _vctParaSour(vct) {
    if (_tran == nullptr) {
      _tran = new Transaction(TranType::AUTOMATE,
                              (uint32_t)Configure::GetAutoTaskOvertime());
    }

    _vctPara.reserve(vct->size());
    for (auto dv : *vct) {
      _vctPara.push_back(dv->Clone());
    }

    _createTime = TimerThread::GetCurrTime();
  }

  ~Statement() {
    if (_bAutoTran && _tran != nullptr) {
      delete _tran;
    }
  }
  /**
   * Get which action will to do
   * @return action type
   */
  virtual ActionType GetActionType() { return ActionType::UNKNOWN; }

  /**
   * Get the field's data type
   * @param paraIndex the field index, start from 0;
   * @return data type
   */
  DataType GetDataType(int paraIndex) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    return _vctPara[paraIndex]->GetDataType();
  };

  /**
   * Get the max length for a field
   * @param paraIndex the field index, start from 0;
   * @return the max length
   */
  int GetMaxLength(int paraIndex) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    return _vctPara[paraIndex]->GetMaxLength();
  }

  /**
   * set a long value
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetLong(int paraIndex, int64_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::LONG)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"LONG", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueLong *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a int value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetInt(int paraIndex, int32_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::INT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"INT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueInt *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a short value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetShort(int paraIndex, int16_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::SHORT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"SHORT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueShort *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a char value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetChar(int paraIndex, char val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::SHORT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"SHORT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueChar *)_vctPara[paraIndex] = val;
  }

  /**
   * set a ulong value
   * @param paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetULong(int paraIndex, uint64_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::ULONG)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"ULONG", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueULong *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a uint value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetUInt(int paraIndex, uint32_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::UINT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"UINT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueUInt *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a ushort value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetUShort(int paraIndex, uint16_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::USHORT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"USHORT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueUShort *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a Byte value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetByte(int paraIndex, Byte val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::BYTE)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"BYTE", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueByte *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a boolean value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetBool(int paraIndex, bool val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::BOOL)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"BOOL", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueBool *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a string value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetString(int paraIndex, const std::string &val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (!_vctPara[paraIndex]->IsStringType())
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"STRING", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    if (val.size() > _vctPara[paraIndex]->GetMaxLength())
      throw ErrorMsg(ErrorID::EXPR_EXCEED_MAX_LENGTH,
                     {to_string(val.size()),
                      to_string(_vctPara[paraIndex]->GetMaxLength())});
    if (_vctPara[paraIndex]->GetDataType() == DataType::FIXCHAR)
      *(DataValueFixChar *)_vctPara[paraIndex] = val;
    else
      *(DataValueVarChar *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a byte array value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param len the length for blob
   * @param val       the value to set
   */
  void SetBlob(int paraIndex, uint32_t len, char *val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::BLOB)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"BLOB", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    if (len > _vctPara[paraIndex]->GetMaxLength())
      throw ErrorMsg(
          ErrorID::EXPR_EXCEED_MAX_LENGTH,
          {to_string(len), to_string(_vctPara[paraIndex]->GetMaxLength())});
    ((DataValueBlob *)_vctPara[paraIndex])->Put(len, val);
  }

  /**
   * Set a double value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetDouble(int paraIndex, double val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::DOUBLE)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"DOUBLE", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueDouble *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a float value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the value to set
   */
  void SetFloat(int paraIndex, float val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::FLOAT)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"FLOAT", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueFloat *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a Date value
   * @param paraIndex paraIndex the field index, start from 0;
   * @param val       the milliseconds from the start of the Clock's epoch.
   */
  void SetDate(int paraIndex, uint64_t val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});
    if (_vctPara[paraIndex]->GetDataType() != DataType::DATETIME)
      throw ErrorMsg(
          ErrorID::EXPR_ERROR_DATATYPE,
          {"DATETIME", StrOfDataType(_vctPara[paraIndex]->GetDataType())});
    *(DataValueDate *)_vctPara[paraIndex] = val;
  }

  /**
   * Set a value by index
   * @param paraIndex fieldName the field name
   * @param val       the value to set
   */
  void SetDataValue(int paraIndex, const IDataValue &val) {
    if (paraIndex < 0 || paraIndex >= _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(paraIndex), to_string(_vctPara.size())});

    _vctPara[paraIndex]->Copy(val);
  }

  /**
   * One time set all fields values, order by fields order
   * @param vals, the data values will be copied, not moved
   */
  void SetValues(const VectorDataValue &vals) {
    if (vals.size() != _vctPara.size())
      throw ErrorMsg(ErrorID::EXPR_INDEX_OUT_RANGE,
                     {to_string(vals.size()), to_string(_vctPara.size())});
    for (int i = 0; i < vals.size(); i++)
      _vctPara[i]->Copy(*vals[i]);
  }

  /**
   * load parameters from byte array, maybe 1~N rows.
   * param bys: The byte array, the struct is below:
   * 4 bytes how many rows in bytes array, 2 bytes how many fields in one row.
   * following are the values for all parameters.
   * param len: bys length, used to verify byte array.
   */
  void LoadParams(Byte *bys, int len) {
    int rows = *(int *)bys;
    bys += sizeof(int);
    len -= sizeof(int);
    int fields = *(short *)bys;
    bys += sizeof(short);
    len -= sizeof(short);

    if (fields != _vctParaSour->size()) {
      throw ErrorMsg(TRAN_ADD_TASK_FAILED, {});
    }

    for (int i = 0; i < rows; i++) {
      for (IDataValue *dv : _vctPara) {
        int rl = dv->ReadData(bys, 0);
        bys += rl;
        len -= rl;
      }
    }

    if (len != 0) {
      throw ErrorMsg(TRAN_ADD_TASK_FAILED, {});
    }
  }
  /**
   * Add current row to a batch array, all rows in a batch will submit one time
   */
  void AddBatch() {
    VectorDataValue *vct = new VectorDataValue();
    vct->swap(_vctPara);
    _vctRow.push_back(vct);

    _vctPara.reserve(_vctParaSour->size());
    for (auto dv : *_vctParaSour) {
      _vctPara.push_back(dv->Clone());
    }
  }

  /**
   * Submit one row or a batch of rows to insert, update or delete
   * @return How much records have been affected.
   */
  virtual int ExecuteUpdate() { abort(); }

  /**
   * Execute the update task asychronous, get the result by future.
   * @return future
   */
  virtual future<int> ExecuteUpdateAsyn() { abort(); }

  /**
   * Execute query statement and return the result
   * @return The query result
   */
  virtual IResultSet *ExecuteQuery() { abort(); }

  /**
   * Execute query task asychronous and get the result by future
   *
   * @return The query result
   */
  virtual future<IResultSet *> ExecuteQueryAsyn() { abort(); }

  /**
   * Close this instance, used to release resource in child class.
   */
  virtual void Close() {}

  DT_MicroSec GetCreateTime() { return _createTime; }
  DT_MicroSec GetStartTime() { return _startTime; }
  DT_MicroSec GetStopTime() { return _stopTime; }
  bool IsFinished() { return _tinyTasks == _finishedTask; }
  uint64_t GetTranId() { return _tran->GetTranId(); }
  TranStatus GetTransactionStatus() { return _tran->GetTransactionStatus(); }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  // If automate transaction, it equal transaction id. If manual transaction, it
  // will start from transaction id to +0xffff
  uint64_t _id;
  // The vactor of data value from sql expression
  const VectorDataValue *_vctParaSour;
  // To save one row of data values
  VectorDataValue _vctPara;
  // To save rows of data values
  VectorRow _vctRow;
  // The create time for this statement
  DT_MicroSec _createTime;
  // The start time to execute for this statement
  DT_MicroSec _startTime;
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime;
  // The number of tiny taks. One statement maybe splite serveral tiny tasks to
  // run. Here used to save how much tiny tasks in total.
  int _tinyTasks = 0;
  // The number of finished tiny tasks.
  int _finishedTask = 0;
  // The transaction to run this task, no nullable.
  Transaction *_tran;
  // If current statement meet error, save the reason here
  ErrorMsg _errorMsg;
  // False, it will input the transaction from outside when construct this
  // statement; True , no transaction incoming and need this statement to create
  // a new transaction.
  bool _bAutoTran = false;
  // All updated records in this statement
  MTreeSet<LeafRecord *>::Type _setRecord;
};
} // namespace storage
