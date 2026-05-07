#pragma once
#include "../cache/Mallocator.h"
#include "../utils/Utilitys.h"
#include "TranEnum.h"

#include <fstream>

namespace storage {
struct SessionGroup;
class Statement;
class Session;
class LogTask;
class RawRecord;
class BranchRecord;

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

  void StartTransaction(bool bAuto, IsoLevel isoLevel = IsoLevel::ReadCommited,
                        CcProtocol ccProtocal = CcProtocol::OCC);
  void AddStatement(Statement *stmt) { _lstStatement.push_back(stmt); }

  MList<Statement *> &GetListStatement() { return _lstStatement; }

  bool IsOvertime() {
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
  bool IsAutoCommit() {
    assert(_tranStatus == TranStatus::AUTO_TRAN ||
           _tranStatus == TranStatus::IN_TRAN);
    return _tranStatus == TranStatus::AUTO_TRAN;
  }
  virtual void SetLogged() { _bLogged.store(true, memory_order_relaxed); }
  bool IsLogged() { return _bLogged.load(memory_order_relaxed); }
  void WriteLog(LogTask *logTask);
  TranID GetTranID() { return _tid; }

  /**
   * @brief Close the transaction and delete all internal statements.
   */
  void CloseTransaction();

  bool IsNeedLog();

  bool IsEmpty() {
    return _lstStatement.size() == 0 && (_tranStatus == TranStatus::INIT ||
                                         _tranStatus == TranStatus::FINISHED);
  }

protected:
  // To generate new TranID, every time it will add 1
  // Transaction ID is 64 bit unsigned integer. The highest 12 bit is node id
  // for distribute system, it can support max 4096 nodes. Following 2 bits is
  // cycle count of system start times, used to avoid transaction repeat. The
  // following 10 bits is used to save session group id. The last 40 bits is
  // used as auto increaseing counter.
  TranID _tid{TXID_NULL};
  // The start time of current transaction
  DT_MicroSec _startTime{UINT64_MAX};
  // The statements executed in this transaction++
  MList<Statement *> _lstStatement;

  // The session own this transaction
  Session *_session;

  TranStatus _tranStatus{TranStatus::INIT};
  IsoLevel _isoLevel{IsoLevel::ReadCommited};
  CcProtocol _ccProtocol{CcProtocol::OCC};
  // The log has been wrote into log files or not
  atomic_bool _bLogged{false};
};

class SplitPageTran : public Transaction {
public:
  SplitPageTran(PageID parentPid, PageID splitPid,
                MVector<BranchRecord *> &&vctNewRec)
      : Transaction(nullptr), _parentPageID(parentPid), _splitPageID(splitPid),
        _vctNewRec(move(vctNewRec)) {}
  void SetLogged() { delete this; }
  PageID GetParentPageID() { return _parentPageID; }
  PageID GetSplitPageID() { return _splitPageID; }
  MVector<BranchRecord *> &GetVctNewRec() { return _vctNewRec; }

protected:
  PageID _parentPageID;
  PageID _splitPageID;
  MVector<BranchRecord *> _vctNewRec;
};
} // namespace storage