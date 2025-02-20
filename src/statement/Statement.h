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
class PhysTable;
class StmtSecRecordAction;

using TreeSetRecord = MTreeSet<LeafRecord *, LeafRecordCmp>;

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
  INIT,     // Just initialized
  INTERVAL, // Unfinished and need to execute again
  SUCEED,   // Succeed to insert
  FAILED,   // Failed to insert due to error or rollback
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
  // The number of LeafRecords that generated for this record
  uint32_t _numLeafRecord{0};
  atomic<ActionStatus> _status{ActionStatus::INIT};
};

// To save secondary LeafRecord selected from secondary index.
struct StmtSecRecord {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  // The LeafRecord from secondary index
  LeafRecord *_secLr;
  Statement *_stmt;
  PhysTable *_table;
  atomic<ActionStatus> _status{ActionStatus::INIT};
  // The number of LeafRecords that generated for this key
  uint32_t _numLeafRecord{0};
  VectorLeafRecord _vctLr;
};

struct QueryRange {
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  QueryRange() {}
  QueryRange(IDataValue *dvLeft, IDataValue *dvRight, bool bRange, bool incLeft,
             bool incRight)
      : _dvLeft(dvLeft), _dvRight(dvRight), _bRange(bRange), _bIncLeft(incLeft),
        _bIncRight(incRight) {}
  QueryRange(const QueryRange &src)
      : _bRange(src._bRange), _bIncLeft(src._bIncLeft),
        _bIncRight(src._bIncRight) {
    if (src._dvLeft != nullptr) {
      _dvLeft = src._dvLeft->AddRef();
      _dvRight = src._dvRight->AddRef();
    }
  }
  QueryRange(QueryRange &&src)
      : _dvLeft(src._dvLeft), _dvRight(src._dvRight), _bRange(src._bRange),
        _bIncLeft(src._bIncLeft), _bIncRight(src._bIncRight) {
    src._dvLeft = nullptr;
    src._dvRight = nullptr;
  }

  ~QueryRange() {
    assert(_bRange || (_dvLeft == nullptr || *_dvLeft == *_dvRight));
    if (_dvLeft != nullptr) {
      _dvLeft->DecRef();
      _dvRight->DecRef();
    }
  }
  QueryRange &operator=(const QueryRange &src) {
    if (_dvLeft != nullptr) {
      _dvLeft->DecRef();
      _dvRight->DecRef();
    }

    _dvLeft = src._dvLeft->AddRef();
    _dvRight = src._dvRight->AddRef();
    _bRange = src._bRange;
    _bIncLeft = src._bIncLeft;
    _bIncRight = src._bIncRight;
    return *this;
  }
  QueryRange &operator=(QueryRange &&src) {
    if (_dvLeft != nullptr) {
      _dvLeft->DecRef();
      _dvRight->DecRef();
    }

    _dvLeft = src._dvLeft;
    _dvRight = src._dvRight;
    _bRange = src._bRange;
    _bIncLeft = src._bIncLeft;
    _bIncRight = src._bIncRight;
    src._dvLeft = nullptr;
    src._dvRight = nullptr;
    return *this;
  }

  // The left range border
  IDataValue *_dvLeft{nullptr};
  // The right range boder, if _bRange=False, it should euqal right border
  IDataValue *_dvRight{nullptr};
  bool _bRange{true};
  bool _bIncLeft{true};  // Include left border, only valid bRange=TRUE
  bool _bIncRight{true}; // Include right boder, only valid bRange=TRUE
};

struct KeyRange {
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

  KeyRange(RawKey *sKey, RawKey *eKey, bool bRange, bool bIncLeft,
           bool bIncRight)
      : _startKey(sKey), _endKey(eKey), _bRange(bRange), _bIncLeft(bIncLeft),
        _bIncRight(bIncRight) {}
  KeyRange(KeyRange &&src)
      : _startKey(src._startKey), _endKey(src._endKey), _bRange(src._bRange),
        _bIncLeft(src._bIncLeft), _bIncRight(src._bIncRight) {
    src._startKey = nullptr;
    src._endKey = nullptr;
  }
  ~KeyRange() {
    delete _startKey;
    delete _endKey;
  }

  RawKey *_startKey{nullptr};
  RawKey *_endKey{nullptr};
  bool _bRange{true};
  bool _bIncLeft{true};  // Include left border, only valid bRange=TRUE
  bool _bIncRight{true}; // Include right boder, only valid bRange=TRUE
};

struct MiddleVar {
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
  // The BrancePage or LeafPage that is handling.
  IndexPage *_midPage{nullptr};
  // The Index key to search, only valid when point search.
  MVector<KeyRange> _vctKeyRange;
  // The PhysTable that the statement is running on
  PhysTable *_table{nullptr};

  int _keyPos{0};
  int _rangePos{-1};
  // True: start from IndexTree range begin;
  // False: start from the position of search key in the range.
  bool _bFromRangeBegin{false};
  // True: Start from page begin;
  // False: Start from the position of search key in the page.
  bool _bFromPageBegin{false};
  // The index position
  int _indexPos{-1};
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
  Statement(uint32_t id, TranID txid, ExprStatement *exprStmt,
            StmtResult *stmtResult)
      : _id(id), _txid(txid), _stmtResult(stmtResult), _exprStmt(exprStmt) {
    _createTime = MicroSecTime();
  }
  Statement(uint32_t id, TranID txid, ExprStatement *exprStmt,
            StmtResult *stmtResult, VectorDataValue &&vctPara)
      : _id(id), _txid(txid), _stmtResult(stmtResult), _exprStmt(exprStmt),
        _vctPara(move(vctPara)) {
    _createTime = MicroSecTime();
  }
  Statement(uint32_t id, TranID txid)
      : _id(id), _txid(txid), _stmtResult(nullptr), _exprStmt(nullptr) {
    _createTime = MicroSecTime();
  }
  virtual ~Statement() {}
  /**
   * @brief Return the expression type
   */
  virtual ExprType GetType() = 0;
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
   * @param rangePos Which range in index that is executing this statement
   * @return True: This method has finished all work and no need to run again.
   * False: There still has no finished work, need to run this method again.
   */
  bool SacnIndex(int rangePos);

  /**
   * @brief To execute the opertion of delete, update, select.
   * @param page The page that the LeafRecord belong to.
   * @param pagePos The position of LeafRecord in LeafPage.
   * @param rangePos Which index range that the page belong to.
   * @return True: The LeafRecord has passed the operation;
   *         false: The LeafRecord has been filter by logic filter
   *         Error: Meet error inoperation, the statement need to set fail
   */
  virtual TriBool HandleLeafRecord(LeafPage *page, int pagePos, int rangePos,
                                   VectorLeafRecord *vctLr = nullptr) {
    abort();
    return TriBool::False;
  }
  /**
   * @brief Collect all LeafRecord for log write. To ensure the last version
   * can be added into set, it should the last statement to call this method
   * first, the first statement should be the last one to call this method.
   * @param setRec: The tree set to save the LeafRecords to write log
   */
  void CollectLogRecords(TreeSetRecord &setRec);
  /**
   * @brief To update RecordStatus into COMMITED of all locked LeafRecord in
   * this statement.
   */
  void Commit();

  /**
   * @brief To update RecordStatus into ROLLBACKED of all locked LeafRecord in
   * this statement.
   */
  void Rollback();

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
  int CalcIndexRanges(IndexTree *idxTree);

  void SetTxID(TranID txid) { _txid = txid; }

  void AddLeafRecords(VectorLeafRecord &vctLr);
  void AddLeafRecord(LeafRecord *lr);

  StmtResult *GetStmtResult() { return _stmtResult; }

  void SetStmtFailed(bool b = true) {
    _stmtFailed.store(b, memory_order_relaxed);
  }

  bool IsStmtFailed() { return _stmtFailed.load(memory_order_relaxed); }

  uint16_t GetSessionGroupId() { return (uint16_t)((_txid >> 40) && 0xFF); }

  void SetStmtStatus(StmtStatus s) { _status = s; }
  StmtStatus GetStmtStatus() { return _status; }

  ExprStatement *GetExprStatement() { return _exprStmt; }

  VectorDataValue &GetParameters() { return _vctPara; }

  void SetFinished(bool b) { _bFinished.store(b, memory_order_release); }

  void SendErrMsg(MString &&errMsg);

protected:
  MVector<QueryRange> MergeAndQueryRange(MVector<QueryRange> &vctLeft,
                                         MVector<QueryRange> &vctRight);

  void MergeOrQueryRange(MVector<QueryRange> &vctResult,
                         MVector<QueryRange> &vctSrc);

  MVector<QueryRange> ConditionConvert(ExprLogic *logic,
                                       VectorDataValue &paras);
  ExprField *GetFieldFromExprLogic(ExprLogic *logic);

  KeyRange GenIndexSearchKey(IndexTree *idxTree, ExprField *field,
                             QueryRange *qRange);

  void SendStmtRecord(int idxPos, int rangePos, PhysTable *table,
                      Statement *stmt, LeafRecord *lr, IndexTree *idxTree);

protected:
  // Id will auto increment 1 every time in self session.
  uint32_t _id;
  // Statement status
  StmtStatus _status{StmtStatus::Created};
  // The index scan has finished for this statement or not
  atomic_bool _bFinished{false};
  // Meet error when executing
  atomic_bool _stmtFailed{false};
  // KeyExec start from the begin of range or search the position by index
  // condition
  bool _bFromBegin{false};

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
  // ExprInsert will be unified managed by a class, do not delete here
  ExprStatement *_exprStmt;
  // The parameters for statement
  VectorDataValue _vctPara;

  MiddleVar *_midVar{nullptr};
};

std::ostream &operator<<(std::ostream &os, const StmtStatus &s);
} // namespace storage
