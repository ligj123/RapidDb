#pragma once
#include "../cache/Mallocator.h"
#include "../statement/Statement.h"
#include "../utils/Utilitys.h"
#include "TranEnum.h"

namespace storage {

struct Transaction {
  void Reset(uint64_t tid, IsoLevel level) : {
    assert(_vctStatement.size() == 0);
    _tid = tid;
    _isoLevel = level;
    _createTime = utils::MicroSecTime();
    _stopTime = UINT64_MAX;
  }

public:
  uint64_t _tid{UINT64_MAX};
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