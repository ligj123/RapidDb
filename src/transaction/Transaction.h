#pragma once
#include "../core/ActionType.h"
#include "../statement/Statement.h"
#include "TranStatus.h"
#include "TranType.h"
#include <atomic>
#include <chrono>

namespace storage {
using namespace std;

class Transaction {
protected:
  // Used to generate id for transaction.
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
  TranStatus _tranStatus;
  // Max milliseconds to finish for this transaction.
  uint32_t _maxMillsSec;
  // To save the failed reason if the transaction failed.
  ErrorMsg _errorMsg;
  // create time
  std::chrono::nanoseconds _createTime;
  // The start time to execute for this statement
  std::chrono::nanoseconds _commitTime;
  // The finished or abort time to execute for this statement
  std::chrono::nanoseconds _stopTime;
};
} // namespace storage
