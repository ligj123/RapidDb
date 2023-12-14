#pragma once
#include "../cache/Mallocator.h"
#include "../core/ActionType.h"
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../expr/BaseExpr.h"
#include "../serv/Transaction.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"

#include <atomic>
#include <chrono>
#include <future>

namespace storage {
class Transaction;
class Statement {
public:
  /**
   * Constuctor for statement
   * @param tran The transaction own this statement, maybe null and create it
   * automately
   * @param paraTmpl Used to save the parameters types, maxlength etc. It will
   * load parameters from byte array according this template
   * @param bTran If need transaction. For normal select, it is not need
   * transaction.
   */
  Statement(uint32_t id, Transaction *tran, ExprStatement *exprStmt,
            const VectorDataValue *paraTmpl)
      : _id(id), _tran(tran), _vctParaTmpl(paraTmpl) {

    _id = _tran->GetTranId() + _tran->GetStatements().size();
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
    if (paraIndex < 0 || paraIndex >= _vctParaTmpl->size())
      return DataType::UNKNOWN;

    return _vctParaTmpl->at(paraIndex)->GetDataType();
  };

  /**
   * Get the max length for a field
   * @param paraIndex the field index, start from 0;
   * @return the max length
   */
  int GetMaxLength(int paraIndex) {
    if (paraIndex < 0 || paraIndex >= _vctParaTmpl->size())
      return -1;
    return _vctParaTmpl->at(paraIndex)->GetMaxLength();
  }

  /**
   * @brief Insert, update, delete or select one or a batch of rows.
   * @return 0: Unfinished, need to run task again, 1: Finished and can return
   * result to client, -1 meet error and the statement failed, need to rollback
   * this statement.
   */
  virtual TaskStatus Execute() { abort(); }

  /**
   * Close this instance, used to release resource in child class.
   */
  virtual void Close() {}

  DT_MicroSec GetCreateTime() { return _createTime; }
  DT_MicroSec GetStartTime() { return _startTime; }
  DT_MicroSec GetStopTime() { return _stopTime; }
  bool IsFinished() { return _tinyTasks == _finishedTask; }
  uint64_t GetTranId() { return _tran->GetTranId(); }

  /**
   * load parameters from byte array, maybe 1~N rows.
   * @param bys: The byte array, the struct is below:
   * 4 bytes how many rows in bytes array, 2 bytes how many fields in one row.
   * following are the values for all parameters. If variable columns, it will
   * start with 4 bytes to save column length. For FIXCHAR type, the parameters
   * will saved as VARCHAR.
   * @param len: bys length, used to verify byte array.
   * @param vctRow: To save the loaded rows of parameters.
   */
  void LoadParams(Byte *bys, int len) {
    Byte *sbys = bys;
    int rows = *(int *)bys;
    bys += sizeof(int);
    short fields = *((short *)bys);
    bys += sizeof(short);

    if (fields != _vctParaTmpl->size()) {
      throw ErrorMsg(STAT_PARAM_NUM_INVALID,
                     {ToMString(_vctParaTmpl->size()), ToMString(fields)});
    }

    for (int i = 0; i < rows; i++) {
      VectorDataValue *vdv = new VectorDataValue;
      for (size_t j = 0; j < _vctParaTmpl->size(); j++) {
        IDataValue *dv = _vctParaTmpl->at(j)->Clone();
        int rl = dv->ReadData(bys);
        vdv->push_back(dv);
        bys += rl;
      }

      _vctParas.push_back(vdv);
    }

    if (len != (int)(bys - sbys)) {
      throw ErrorMsg(STAT_PARAM_LEN_INVALID,
                     {ToMString(len), ToMString((int)(bys - sbys))});
    }
  }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  // Id will auto increment 1 every time in self session.
  uint32_t _id;
  // The expr statement
  ExprStatement *_exprStmt;
  // To save multi rows of parameters loaded from client byte array
  VectorRow _vctParas;
  // The create time for this statement
  DT_MicroSec _createTime;
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime = 0;
  // The transaction to run this task, no nullable.
  Transaction *_tran;
  // If current statement meet error, save the reason here
  unique_ptr<ErrorMsg> _errorMsg = nullptr;
  // All inserted, updated or locked records in this statement
  VectorLeafRecord _vctRecord;
};
} // namespace storage
