#include "Transaction.h"
#include "../statement/Statement.h"

namespace storage {
atomic_uint64_t Transaction::_atomicAutoTranId = (SecondTime() & 0xffffff)
                                                 << 24;
atomic_uint64_t Transaction::_atomicManualTranId = (SecondTime() & 0xffffff)
                                                   << 24;
SpinMutex Transaction::_mutexTran;
MHashMap<uint64_t, Transaction *> Transaction::_mapTransaction;
thread_local ErrorMsg *Transaction::_localErrorMsg = nullptr;

void Transaction::SetTransactionstatus(TranStatus status) {
  if (status == TranStatus::COMMITTING) {
    assert(_tranStatus == TranStatus::CREATED);
    _startTime = MicroSecTime();
  } else {
    _stopTime = MicroSecTime();

#ifdef DEBUG_TEST
    if (status == TranStatus::CLOSED) {
      assert(_tranStatus == TranStatus::COMMITTING);
    } else if (status == TranStatus::FAILED) {
      assert(_tranStatus == TranStatus::COMMITTING);
    } else if (status == TranStatus::ROLLBACK) {
      assert(_tranStatus == TranStatus::CREATED ||
             _tranStatus == TranStatus::COMMITTING);
    } else if (status == TranStatus::ROLLBACK) {
      assert(_tranStatus == TranStatus::CREATED ||
             _tranStatus == TranStatus::COMMITTING);
    }
#endif
  }

  _tranStatus = status;
}

void Transaction::Rollback() {}

void Transaction::Failed() {}

void Transaction::TimeOut() {}

void Transaction::Commited() {}
} // namespace storage
