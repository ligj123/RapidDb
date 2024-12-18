#include "Transaction.h"

#include "SessionPool.h"

namespace storage {
void Transaction::StartTransaction(bool bAuto, IsoLevel isoLevel,
                                   CcProtocol ccProtocol) {
  if (bAuto) {
    _tranStatus = TranStatus::AUTO_TRAN;
  } else {
    _tranStatus = TranStatus::IN_TRAN;
  }

  _isoLevel = isoLevel;
  _ccProtocol = ccProtocol;

  SessionGroup &group = SessionPool::GetSessionGroup(_sessionGroupId);
  _tid = group._currTranId;
  if (group._currTranId & 0xFFFFFFFFFF == 0xFFFFFFFFFF) [[unlikely]] {
    group._currTranId &= 0xFFFFFF0000000000;
  } else {
    group._currTranId++;
  }

  _startTime = MicroSecTime();
}

} // namespace storage