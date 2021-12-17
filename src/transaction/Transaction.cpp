#include "Transaction.h"
#include "../statement/Statement.h"

namespace storage {
atomic_uint64_t Transaction::atomicTranId = (MSTime() & 0xffffff) << 24;

void Transaction::SetTransactionstatus(TranStatus status) {
  if (status == TranStatus::COMMITTING) {
    assert(_tranStatus == TranStatus::CREATED);
    if (_bStatTime)
      _startTime = std::chrono::system_clock::now();
  } else if (status == TranStatus::CLOSED) {
    assert(_tranStatus == TranStatus::COMMITTING);
    if (_bStatTime)
      _stopTime = std::chrono::system_clock::now();
  } else if (status == TranStatus::FAILED) {
    assert(_tranStatus == TranStatus::COMMITTING);
    if (_bStatTime)
      _stopTime = std::chrono::system_clock::now();
  } else if (status == TranStatus::ROLLBACK) {
    assert(_tranStatus == TranStatus::CREATED ||
           _tranStatus == TranStatus::COMMITTING);
    _stopTime = std::chrono::system_clock::now();
  } else if (status == TranStatus::ROLLBACK) {
    assert(_tranStatus == TranStatus::CREATED ||
           _tranStatus == TranStatus::COMMITTING);
    _stopTime = std::chrono::system_clock::now();
  } else if (status == TranStatus::CLOSED) {
    assert(_tranStatus == TranStatus::COMMITTING);
    _stopTime = std::chrono::system_clock::now();
  }

  _tranStatus = status;
}

void Transaction::Rollback() {}

void Transaction::Failed() {}

void Transaction::TimeOut() {}

void Transaction::Commited() {}
} // namespace storage
