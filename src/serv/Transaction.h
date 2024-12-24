#pragma once
#include "../cache/Mallocator.h"
#include "../statement/Statement.h"
#include "../utils/Utilitys.h"
#include "TranEnum.h"

namespace storage {
class Statement;
struct SessionGroup;

class Transaction {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  Transaction(Session *session) : _session(session) {}

  void StartTransaction(SessionGroup &sGroup, bool bAuto,
                        IsoLevel isoLevel = IsoLevel::ReadCommited,
                        CcProtocol ccProtocal = CcProtocol::OCC);
  void AddStatement(Statement *stmt) { _vctStatement.push_back(stmt); }

  MVector<Statement *> &GetVctStatement() { return _vctStatement; }

  bool IsTranOvertime() {
    assert(_tranStatus == TranStatus::AUTO_TRAN ||
           _tranStatus == TranStatus::IN_TRAN);
    if (_tranStatus == TranStatus::AUTO_TRAN) {
      if (MicroSecTime() - _startTime > Configure::GetAutoTranOvertime())
        return true;
    } else {
      if (MicroSecTime() - _startTime > Configure::GetMultiTranOvertime())
        return true;
    }

    return false;
  }

  TranStatus GetTranStatus() { return _tranStatus; }
  void SetTranStatus(TranStatus s) { _tranStatus = s; }
  TranType GetTranType() { return _tranType; }

protected:
  TranID _tid{TXID_NULL};
  // The start time of current transaction
  DT_MicroSec _startTime{UINT64_MAX};
  // The statements executed in this transaction++
  MVector<Statement *> _vctStatement;

  // The session own this transaction
  Session *_session;

  TranStatus _tranStatus{TranStatus::INIT};
  TranType _tranType{TranType::AUTOMATE};
  IsoLevel _isoLevel{IsoLevel::ReadCommited};
  CcProtocol _ccProtocol{CcProtocol::OCC};
};

} // namespace storage