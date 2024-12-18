#pragma once
#include "../cache/Mallocator.h"
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../expr/BaseExpr.h"
#include "../utils/ErrorID.h"
#include "../utils/ErrorMsg.h"
#include "../utils/Utilitys.h"
#include "StmtResult.h"

#include <atomic>
#include <chrono>
#include <future>

namespace storage {
class LeafRecord;

enum class StmtStatus : uint8_t {
  Created,   // Just create this statement and NOT start to execute
  Executing, // The statement has been added into task to wait to execute or
             // executing.
  Executed,  // The statement has been executed and can go to next step.
  Logging,   // Collecting log and wait the logs to be write into files.
  Logged,    // Have finished to write log into files.
  Finished,  // Have executed and wrote log if needed, send commit ot abort
             // singal to all LeafRecord. if the end user got the result, this
             // statement can be freed.
};

class Statement {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  /**
   * Constuctor for statement
   * @param id The id of this statement, auto increment 1 in every session.
   * @param tran The transaction own this statement.
   */
  Statement(uint32_t id, TranID txid) : _id(id), _txid(txid) {
    _createTime = MicroSecTime();
  }

  virtual ExprType GetActionType() = 0;
  /**
   * @brief To be called in session group, to check if current step has finished
   * and can go to next step.
   */
  virtual StmtStatus CheckStatus() = 0;
  /**
   * @brief Execute this statement
   * @return True: This statement has finished and can go to next step.
   False:
   * Need to exec again or failed if _errorMsg != nullptr.
   */
  virtual bool Exec() {
    abort();
    return false;
  }
  /**
   * @brief Collect all LeafRecord for log write
   * @param setRec: The tree set to save the LeafRecords to write log
   */
  virtual void CollectLogRecords(MTreeSet<LeafRecord *> &setRec) {
    // For readonly statement, it has not records that need to write log.
    abort();
  }
  /**
   * @brief To update RecordStatus into COMMITED of all locked LeafRecord in
   * this statement.
   */
  virtual void Commit() { assert(false); }

  /**
   * @brief To update RecordStatus into ROLLBACKED of all locked LeafRecord in
   * this statement.
   */
  virtual void Rollback() { assert(false); }

  /**The statement is readonly or not */
  virtual bool IsReadonly() = 0;

  DT_MicroSec GetCreateTime() { return _createTime; }
  DT_MicroSec GetStopTime() { return _stopTime; }
  TranID GetTxId() { return _txid; }
  uint32_t GetId() { return _id; }

  /**
   * @brief To calc which index range for this statement.
   * @param idxTree The IndexTree
   * @return Which range that this statement belong to.
   */
  virtual int CalcIndexRange(IndexTree *idxTree) {
    abort();
    return -1;
  }

  void SeReadResult(bool b) { _readResult = b; }
  bool GetReadResult() { return _readResult; }

  void AddLeafRecord(LeafRecord *lr);

protected:
  // Id will auto increment 1 every time in self session.
  uint32_t _id;
  // Statement status
  StmtStatus _status;
  // The end user has read the result or not
  bool _readResult;
  // Meet error when executing
  atomic_bool _stmtFailed{false};
  // The create time for this statement
  DT_MicroSec _createTime;
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime = 0;
  // The transaction id to run this task, must be valid.
  TranID _txid;
  // If current statement meet error, save the reason here
  unique_ptr<ErrorMsg> _errorMsg = nullptr;
  // Warning messages
  vector<ErrorMsg> _vctWarnMsg;

  // All LeafRecords that just created and are not added into LeafPages.
  MList<LeafRecord *> _lstWaitRecord;
  // The LeafRecords that has been added into LeafPages or have error.
  MList<LeafRecord *> _lstFinshRecord;

  StmtResult _stmtResult;
};
} // namespace storage
