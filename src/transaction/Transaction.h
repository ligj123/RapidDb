#pragma once
#include "../core/ActionType.h"
#include "../core/LeafRecord.h"
#include "../header.h"
#include "../statement/Statement.h"
#include "../utils/SpinMutex.h"
#include "TranStatus.h"
#include "TranType.h"
#include <atomic>
#include <chrono>

namespace storage {
using namespace std;
using namespace std::chrono;

class Transaction {
public:
  Transaction(TranType tType, uint32_t maxMillsSec)
      : _tranType(tType), _maxMillsSec(_maxMillsSec) {
    _createTime = system_clock::now();
    uint64_t nano =
        (duration_cast<nanoseconds>(_createTime.time_since_epoch())).count();
    _tranId = ((nano / NANO_SEC) & (1 << 24 - 1)) << 24;
    _tranId += atomicTranId.fetch_add(1) & (1 << 24 - 1);
  }

public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

protected:
  // Used to generate id for transactions.
  static atomic_uint atomicTranId;
  // To save transactions until close them
  static MVector<Transaction *> _vctTransaction;

protected:
  // Transaction id
  // One transaction is 64 bits uint64_t, the highest 16 bits is reserve for
  // distributed version. The following 24 bits is remainder of seconds since
  // epoch. The last 24 bit is self increasing integer from atomicTranId.
  uint64_t _tranId;
  TranType _tranType;
  TranStatus _tranStatus = TranStatus::CREATED;
  // Max milliseconds to finish for this transaction.
  uint32_t _maxMillsSec;
  // To save the failed reason if the transaction failed.
  ErrorMsg *_errorMsg = nullptr;
  // Create time
  time_point<system_clock> _createTime;
  // The start time to execute for this statement
  time_point<system_clock> _commitTime;
  // The finished or abort time to execute for this statement
  time_point<system_clock> _stopTime;

  list<Statement *> _lstStatement;
  list<LeafRecord *> _lstRecord;

  SpinMutex _spinMutex;
};
} // namespace storage
