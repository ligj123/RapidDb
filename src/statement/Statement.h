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
class IndexTree;
class IndexAction;

enum class StmtStatus : uint8_t {
  Created,     // Just create this statement and NOT start to execute
  Initialized, // Finished to execute in SessionTask and The related data has
               // been send to IndexTask queue.
  Executing,   // The statement is executing in IndexTask
  Executed, // The statement has been executed in IndexTask and can go to next
            // step.
  Logging, // Collecting log and wait the logs write thread to send back result.
  Logged,  // Have received the result from log write thread.
  Finished, // All tasks have finished in this statement, include log write,
            // commit (or rollback), The statement can be delete.
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
  Statement(uint32_t id, TranID txid, StmtResult *stmtResult = nullptr)
      : _id(id), _txid(txid), _stmtResult(stmtResult) {
    _createTime = MicroSecTime();
  }

  virtual ExprType GetActionType() = 0;
  /**
   * @brief To be called in session group, to check if current step has finished
   * and can go to next step.
   */
  virtual StmtStatus CheckStatus() {
    abort();
    return StmtStatus::Finished;
  }
  /**
   * @brief Execute this statement in SessionTask
   * @return True: This statement has finished and can go to next step.
   False:
   * Need to exec again or failed if _errorMsg != nullptr.
   */
  virtual bool SessionExec() {
    abort();
    return false;
  }

  /**
   * @brief Execute this statement in primary key IndexTask
   * @param rangePos The range position of IndexTask to call this method
   * @return True: This statement has finished and can go to next step.
   False:
   * Need to exec again or failed if _errorMsg != nullptr.
   */
  virtual bool PrimaryKeyExec(int rangePos) {
    abort();
    return false;
  }

  /**
 * @brief Execute this statement in secondary key IndexTask
 * @param rangePos The range position of IndexTask to call this method
 * @return True: This statement has finished and can go to next step.
 False:
 * Need to exec again or failed if _errorMsg != nullptr.
 */
  virtual bool SecondaryKeyExec(int rangePos) {
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
   * @brief Get the index ranges in the IndexTree.
   * @param idxTree The IndexTree
   * @return The ranges that this statement need to exec.
   */
  virtual int GetIndexRange(IndexTree *idxTree) {
    abort();
    return {};
  }

  void SetTxID(TranID txid) { _txid = txid; }

  void AddLeafRecord(LeafRecord *lr);

  StmtResult *GetStmtResult() { return _stmtResult; }

  void SetStmtFailed(bool b = true) {
    _stmtFailed.store(b, memory_order_relaxed);
  }

protected:
  // Id will auto increment 1 every time in self session.
  uint32_t _id;
  // Statement status
  StmtStatus _status;
  // Meet error when executing
  atomic_bool _stmtFailed{false};
  // The create time for this statement
  DT_MicroSec _createTime;
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime = 0;
  // The transaction id to run this task, must be valid.
  TranID _txid;

  // All LeafRecords that just created and are not added into LeafPages.
  MList<LeafRecord *> _lstWaitRecord;
  // The LeafRecords that has been added into LeafPages or have error.
  MList<LeafRecord *> _lstFinshRecord;
  // Return the result to end user
  StmtResult *_stmtResult;
};
} // namespace storage
