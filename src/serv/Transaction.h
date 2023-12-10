#pragma once
#include "../cache/Mallocator.h"
#include "../statement/Statement.h"
#include "../utils/Utilitys.h"

namespace storage {

struct Transaction {
  void Reset(uint64_t tid) {
    assert(_vctStatement.size() == 0);
    _tid = tid;
    _createTime = utils::MicroSecTime();
    _stopTime = UINT64_MAX;
  }

public:
  uint64_t _tid{UINT64_MAX};
  // Create time
  DT_MicroSec _createTime{UINT64_MAX};
  // The finished or abort time to execute for this statement
  DT_MicroSec _stopTime{UINT64_MAX};
  // The statements executed in this transaction
  MVector<Statement *> _vctStatement;
};
} // namespace storage