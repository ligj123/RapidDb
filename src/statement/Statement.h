#pragma once
#include "../cache/Mallocator.h"
#include "../core/RawKey.h"
#include "../dataType/DataType.h"
#include "../dataType/IDataValue.h"
#include "../expr/BaseExpr.h"
#include "../serv/Session.h"
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
struct LeafRecordCmp;
class Statement;

enum class StmtStatus : uint8_t {
  Created,   // Just create this statement and NOT start to execute
  Executing, // The statement is executing in IndexTask
  Executed,  // The statement has been executed in IndexTask and can go to
             // next step.
  Logging,   // Collecting log and wait the logs write thread to send back
             // result.
  Logged,    // Have received the result from log write thread.
  Finished,  // All tasks have finished in this statement, include log write,
             // commit (or rollback), The statement can be delete.
};

enum class ActionStatus {
  INIT,   // Just initialized
  SUCEED, // Succeed to insert
  FAILED, // Failed to insert due to error or rollback
};

// To save the handles paras in InsertStatement.
struct StmtInsertRecord {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  StmtInsertRecord(RawKey &&priKey, VectorDataValue &&vctParas, Statement *stmt,
                   PhysTable *table)
      : _priKey(move(priKey)), _vctParas(move(vctParas)), _stmt(stmt),
        _table(table) {}

  RawKey _priKey;            // Primary key of the record
  VectorDataValue _vctParas; // The columns' values of this record
  Statement *_stmt;
  PhysTable *_table;
  uint32_t _numLeafRecord; // The number of LeafRecords that generated for
                           // this record
  atomic<ActionStatus> _status{ActionStatus::INIT};
};

// To save primary key selected from secondary index.
struct StmtPriKey {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  RawKey _priKey;
  Statement *_stmt;
  ActionStatus _status{ActionStatus::INIT};
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
   * @brief Constuctor for statement
   * @param id The id of this statement, auto increment 1 in every session.
   * @param tran The transaction own this statement.
   * @param stmtResult The pointer of statement result
   */
  Statement(uint32_t id, TranID txid, StmtResult *stmtResult = nullptr)
      : _id(id), _txid(txid), _stmtResult(stmtResult) {
    _createTime = MicroSecTime();
  }
  virtual ~Statement() {}
  /**
   * @brief Return the expression type
   */
  virtual ExprType GetType() = 0;
  /**
   * @brief To be called in session group, to check if current step has
   * finished and can go to next step.
   */
  virtual StmtStatus CheckStatus() {
    abort();
    return StmtStatus::Finished;
  }
  /**
   * @brief Execute this statement in SessionTask
   * @param sess The session that this statement belong to
   * @return True: This method has finished all work and need not to run
   * again. False: There still has no finished work, need to run this method
   * again.
   */
  virtual StmtStatus SessionExec(Session *sess) {
    abort();
    return StmtStatus::Finished;
  }

  /**
   * @brief Execute this statement in primary key IndexTask
   * @param rangePos The range position of IndexTask to call this method
   * @return True: This method has finished all work and need not to run
   * again. False: There still has no finished work, need to run this method
   * again.
   */
  virtual bool PrimaryKeyExec(int rangePos) {
    abort();
    return false;
  }

  /**
   * @brief Execute this statement in secondary key IndexTask
   * @param rangePos The range position of IndexTask to call this method
   * @return True: This method has finished all work and need not to run
   * again. False: There still has no finished work, need to run this method
   * again.
   */
  virtual bool SecondaryKeyExec(int rangePos) {
    abort();
    return false;
  }

  /**
   * @brief Collect all LeafRecord for log write. To ensure the last version
   * can be added into set, it should the last statement to call this method
   * first, the first statement should be the last one to call this method.
   * @param setRec: The tree set to save the LeafRecords to write log
   */
  virtual void
  CollectLogRecords(MTreeSet<LeafRecord *, LeafRecordCmp> &setRec) {
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
  virtual int CalcIndexRanges(IndexTree *idxTree) {
    abort();
    return -1;
  }

  void SetTxID(TranID txid) { _txid = txid; }

  void AddLeafRecord(LeafRecord *lr);

  StmtResult *GetStmtResult() { return _stmtResult; }

  void SetStmtFailed(bool b = true) {
    _stmtFailed.store(b, memory_order_relaxed);
  }

  bool IsStmtFailed() { return _stmtFailed.load(memory_order_relaxed); }

  uint16_t GetSessionGroupId() { return (uint16_t)((_txid >> 40) && 0xFF); }

  void SetStmtStatus(StmtStatus s) { _status = s; }
  StmtStatus GetStmtStatus() { return _status; }

protected:
  // Id will auto increment 1 every time in self session.
  uint32_t _id;
  // Statement status
  StmtStatus _status{StmtStatus::Created};
  // Meet error when executing
  atomic_bool _stmtFailed{false};
  // The create time for this statement
  DT_MicroSec _createTime;
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime{0};
  // The transaction id to run this task, must be valid.
  TranID _txid;

  // All LeafRecords that just created and are not added into LeafPages.
  MList<LeafRecord *> _lstWaitRecord;
  // The LeafRecords that has been added into LeafPages or have error.
  MList<LeafRecord *> _lstFinishRecord;
  // Return the result to end user
  StmtResult *_stmtResult;
};

std::ostream &operator<<(std::ostream &os, const StmtStatus &s);
} // namespace storage
