#pragma once
#include "../cache/Mallocator.h"
#include "../core/ActionType.h"
#include "../core/LeafRecord.h"
#include "../header.h"
#include "../utils/SpinMutex.h"
#include "../utils/Utilitys.h"
#include "TranStatus.h"
#include "TranType.h"
#include <atomic>
#include <chrono>

namespace storage {
using namespace std;
using namespace std::chrono;
class Statement;
class Transaction {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }
  static void PushTransaction(Transaction *tran) {
    lock_guard<SpinMutex> lock(_mutexTran);
    _mapTransaction.insert({tran->_tranId, tran});
  }
  static void EraseTransacton(uint64_t id) {
    lock_guard<SpinMutex> lock(_mutexTran);
    Transaction *tran = _mapTransaction.find(id)->second;
    delete tran;
  }
  static void SetLocalErrorMsg(ErrorMsg *msg) {
    if (_localErrorMsg != nullptr) {
      delete _localErrorMsg;
    }

    _localErrorMsg = msg;
  }
  static ErrorMsg *GetLocalErrorMsg() { return _localErrorMsg; }

public:
  Transaction(TranType tType, uint32_t maxMillsSec, bool bStatTime)
      : _tranType(tType), _maxMillsSec(maxMillsSec), _bStatTime(bStatTime) {
    if (bStatTime)
      _createTime = system_clock::now();

    if (tType == TranType::AUTOMATE) {
      _tranId = _atomicAutoTranId.fetch_add(1, memory_order_relaxed);
      if ((_tranId & 0xffffff) == 0xffff00) {
        _atomicAutoTranId.store((MSTime() & 0xffffff) << 24);
      }
    } else {
      _tranId = _atomicManualTranId.fetch_add(0xff, memory_order_relaxed);
      if ((_tranId & 0xffffff) == 0xfff000) {
        _atomicAutoTranId.store((MSTime() & 0xffffff) << 24);
      }
    }
  }
  ~Transaction() {}

  TranType GetTransactionType() const { return _tranType; }
  TranStatus GetTransactionStatus() const { return _tranStatus; }
  const ErrorMsg *GetErrorMsg() const { return _errorMsg; }
  void SetTransactionstatus(TranStatus status);
  bool IsOvertime() {
    if (_bStatTime) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now() - _createTime)
                 .count() > _maxMillsSec;
    } else {
      return false;
    }
  }
  uint64_t GetTranId() { return _tranId; }
  bool AbleAddTask() { return _tranStatus == TranStatus::CREATED; }
  void Rollback();
  void Failed();
  void TimeOut();
  void Commited();

protected:
  // Used to generate id for automated transactions.
  static atomic_uint64_t _atomicAutoTranId;
  // Used to generate id for manual transactions.
  static atomic_uint64_t _atomicManualTranId;
  // To lock atomicTranId and generate new transaction id
  static SpinMutex _mutexTran;
  // To keep transactions until close them
  static MHashMap<uint64_t, Transaction *>::Type _mapTransaction;
  // thread local error msg, it will be replaced by following error
  static thread_local ErrorMsg *_localErrorMsg;

protected:
  TranType _tranType;
  TranStatus _tranStatus = TranStatus::CREATED;
  // If to save create, commit, stop time
  bool _bStatTime;
  // Max milliseconds to finish for this transaction.
  uint32_t _maxMillsSec;
  // Transaction id
  // One transaction is 64 bits uint64_t. Thee highest bit is automate(0) or
  // manual(1) type. The following 15 bits is reserve for distributed version,
  // it will related to node. The following 24 bits is remainder of seconds
  // since epoch. The last 24 bit is self increasing integer from atomicTranId.
  // For automate type, one transaction only has one statement, so tran id will
  // increase 1 every time. For manual type, One tansaction can has many
  // statements, so every time tran id will increase 256 to accept more
  // statements.
  uint64_t _tranId;
  // To save the failed reason if the transaction failed or nullptr.
  ErrorMsg *_errorMsg = nullptr;
  // Create time
  time_point<system_clock> _createTime;
  // The start time to execute for this statement
  time_point<system_clock> _startTime;
  // The finished or abort time to execute for this statement
  time_point<system_clock> _stopTime;
  // All statements in this transaction
  MTreeSet<Statement *>::Type _setStatement;

  // To record the number of finished records
  uint64_t _recFinished = 0;
  // spin lock
  SpinMutex _spinMutex;
};
} // namespace storage
