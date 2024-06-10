#pragma once
#include "../cache/Mallocator.h"
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
class LeafRecord;

struct LeafRecordAction {
public:
  // LeafRecord to insert or delete
  LeafRecord *_leafRecord;
  // Used to temporary save the index page if the page is not in memory
  IndexPage *_midPage{nullptr};
};

class Statement {
public:
  /**
   * Constuctor for statement
   * @param id The id of this statement, auto increment 1 in every session.
   * @param tran The transaction own this statement.
   */
  Statement(uint32_t id, Transaction *tran) : _id(id), _tran(tran) {
    _createTime = TimerThread::GetCurrTime();
  }

  virtual ExprType GetActionType() = 0;
  /**
   * @brief Execute this statement
   * @return True: This statement has finished and can go to next step. False:
   * Need to exec again or failed if _errorMsg != nullptr.
   */
  virtual bool Exec() = 0;
  /**
   * @brief Collect all LeafRecord and sort them from the statement one by one.
   * @param setRec: The tree set to save the LeafRecords to write log
   */
  virtual void CollectRecords(MTreeSet<LeafRecord *> &setRec) {
    // For readonly statement, it has not records that need to write log.
  }
  /**
   * @brief To update RecordStatus into COMMIT of all locked LeafRecord in this
   * statement.
   */
  virtual void Commit() { assert(false); }
  /**
   * @brief To update RecordStatus into ROLLBACK of all locked LeafRecord in
   * this statement.
   */
  virtual void Rollback() { assert(false); }
  /**The statement is readonly or not */
  virtual bool IsReadonly() = 0;

  DT_MicroSec GetCreateTime() { return _createTime; }
  DT_MicroSec GetStopTime() { return _stopTime; }
  Transaction *GetTransaction() { return _tran; }

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
  // The create time for this statement
  DT_MicroSec _createTime;
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime = 0;
  // The transaction to run this task, no nullable.
  Transaction *_tran;
  // If current statement meet error, save the reason here
  unique_ptr<ErrorMsg> _errorMsg = nullptr;
  // Warning messages
  vector<ErrorMsg> _vctWarnMsg;
};
} // namespace storage
