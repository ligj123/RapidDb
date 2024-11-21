#pragma once
#include "../cache/Mallocator.h"
#include "../statement/Statement.h"
#include "../utils/Utilitys.h"
#include "TranEnum.h"

namespace storage {
class Statement;

struct Transaction {
public:
  static void *operator new(size_t size) {
    return CachePool::Apply((uint32_t)size);
  }
  static void operator delete(void *ptr, size_t size) {
    CachePool::Release((Byte *)ptr, (uint32_t)size);
  }

public:
  void Reset(TranID tid, IsoLevel level) {
    assert(_vctStatement.size() == 0);
    _tid = tid;
    _isoLevel = level;
    _createTime = MicroSecTime();
  }

public:
  TranID _tid{TXID_NULL};
  // Create time
  DT_MicroSec _createTime{UINT64_MAX};
  // The finished or abort time to execute for this statement
  DT_MicroSec _endTime{UINT64_MAX};
  // The statements executed in this transaction++
  MVector<Statement *> _vctStatement;
  // spin lock
  SpinMutex _spinMutex;
  // The session own this transaction
  Session *_session;

  TranStatus _tranStatus;
  TranType _tranType;
  IsoLevel _isoLevel;
};

} // namespace storage