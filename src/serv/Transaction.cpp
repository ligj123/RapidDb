#include "Transaction.h"

#include "SessionPool.h"

namespace storage {
void Transaction::StartTransaction(SessionGroup &sGroup, bool bAuto,
                                   IsoLevel isoLevel, CcProtocol ccProtocol) {
  if (bAuto) {
    _tranStatus = TranStatus::AUTO_TRAN;
  } else {
    _tranStatus = TranStatus::IN_TRAN;
  }

  _isoLevel = isoLevel;
  _ccProtocol = ccProtocol;

  _tid = sGroup._currTranId;
  if (sGroup._currTranId & 0xFFFFFFFFFF == 0xFFFFFFFFFF) [[unlikely]] {
    sGroup._currTranId &= 0xFFFFFF0000000000;
  } else {
    sGroup._currTranId++;
  }

  _startTime = MicroSecTime();
}

} // namespace storage