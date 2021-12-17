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

    _tranId = atomicTranId.fetch_add(1);
    if ((_tranId & 0xffffff) >= 0xfff000) {
      if ((_tranId & 0xffffff) >= 0xfff000) {
        _tranId = (MSTime() & 0xffffff) << 24;
        atomicTranId.store(_tranId + 1);
      } else {
        while (true) {
          std::this_thread::yield();
          _tranId = atomicTranId.fetch_add(1);
          if ((_tranId && 0xffffff) < 0xfff000)
            break;
        }
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
  // Used to generate id for transactions.
  static atomic_uint64_t atomicTranId;
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
  // One transaction is 64 bits uint64_t, the highest 16 bits is reserve for
  // distributed version. The following 24 bits is remainder of seconds since
  // epoch. The last 24 bit is self increasing integer from atomicTranId.
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
  MList<Statement *>::Type _lstStatement;
  // All updated records in this transaction
  MTreeSet<LeafRecord *>::Type _setRecord;
  // To record the number of finished records
  uint64_t _recFinished = 0;
  // spin lock
  SpinMutex _spinMutex;
};
} // namespace storage
