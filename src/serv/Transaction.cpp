#include "Transaction.h"

#include "../binlog/LogTask.h"
#include "../statement/Statement.h"
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

  vector<SessionGroup> &vctGroup = SessionPool::GetVctSessionGroup();
  SessionGroup &sGroup = vctGroup[_session->_id % vctGroup.size()];
  _tid = sGroup._currTranId;
  if ((sGroup._currTranId & 0xFFFFFFFFFF) == 0xFFFFFFFFFF) [[unlikely]] {
    sGroup._currTranId &= 0xFFFFFF0000000000;
  } else {
    sGroup._currTranId++;
  }

  _startTime = MicroSecTime();
}

void Transaction::WriteLog(LogTask *logTask) {
  // Now does not consider DDL statement
  bool bStart = true;

  Byte *sBuff = logTask->GetBuff();
  Byte *cBuff = sBuff;
  *((uint64_t *)cBuff) = _tid;
  cBuff += UI64_LEN;

  TreeSetRecord setRec;
  for (auto iter = _lstStatement.rbegin(); iter != _lstStatement.rend();
       iter++) {
    (*iter)->CollectLogRecords(setRec);
  }

  *((uint32_t *)cBuff) = (uint32_t)setRec.size();
  cBuff += UI32_LEN;

  for (LeafRecord *lr : setRec) {
    uint16_t len = lr->GetActualLength();
    if (cBuff - sBuff > BUFF_SIZE - len) {
      logTask->WriteBuff(cBuff - sBuff, bStart);
      bStart = false;
      cBuff = sBuff;
    }

    BytesCopy(cBuff, lr->GetBysValue(), len);
    cBuff += len;

    if (lr->HasOverflowPage()) {
      OverflowPage *ovPage = lr->GetOverflowPage();
      if (cBuff - sBuff > BUFF_SIZE - ovPage->PageSize()) {
        logTask->WriteBuff(cBuff - sBuff, bStart);
        bStart = false;
        cBuff = sBuff;
      }

      BytesCopy(cBuff, ovPage->GetBysPage(), ovPage->PageSize());
      cBuff += ovPage->PageSize();
    }
  }

  logTask->WriteBuff(cBuff - sBuff, bStart);
}

} // namespace storage