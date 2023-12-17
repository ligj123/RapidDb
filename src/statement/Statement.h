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
            VectorRow &vctPara)
      : _id(id), _tran(tran), _vctParas(std::move(vctPara)) {
    _createTime = TimerThread::GetCurrTime();
  }

  /**
   * Close this instance, used to release resource in child class.
   */
  virtual void Exec() = 0;
  virtual void WriteLog() = 0;
  virtual void Commit() = 0;

  DT_MicroSec GetCreateTime() { return _createTime; }
  DT_MicroSec GetStartTime() { return _startTime; }
  DT_MicroSec GetStopTime() { return _stopTime; }
  uint64_t GetTranId() { return _tran->GetTranId(); }

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
