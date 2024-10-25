#include "TableTaskMgr.h"

#include "IndexTask.h"

namespace storage {

inline uint16_t JudgePriKeyRange(const RawKey &key,
                                 const MVector<LeafRecord> &_vctRecBorder) {
  return 0;
}

inline uint16_t JudgeLeafRecordRange(const LeafRecord &lr,
                                     const MVector<LeafRecord> &_vctRecBorder) {
  return 0;
}

void TableTaskMgr::CollectPrimaryTaskData() {
  if (!_priIndexTaskQueue._spinMutex.try_lock())
    return;

  MDeque<Statement *> qs;
  _priIndexTaskQueue._fqStmt.Pop(qs);

  for (auto iter = qs.begin(); iter != qs.end(); iter++) {
    Statement *stmt = *iter;
    qs.pop_back();

    MVector<uint16_t> vs = stmt->SpliteRange(_priIndexTaskQueue._vctRecBorder);
    for (uint16_t pos : vs) {
      _vctPriIndexTask[pos]._lqStmt.Push(stmt);
    }
  }

  for (size_t i = 0; i < _vctPriIndexTask.size(); i++) {
    _vctPriIndexTask[i]._lqStmt.Submit();
  }

  for (SecIndexTaskQueue sitq : _vctSecIndexTaskQueue) {
    MDeque<PriKeyStmt *> qp;
    sitq._fqPriKeyStmt.Pop(qp);

    for (auto iter = qp.begin(); iter != qp.end(); iter++) {
      PriKeyStmt *pks = *iter;
      uint16_t pos =
          JudgePriKeyRange(pks->_key, _priIndexTaskQueue._vctRecBorder);
      _vctPriIndexTask[pos]._lqPriKeyStmt.Push(pks, false);
    }
  }

  for (size_t i = 0; i < _vctPriIndexTask.size(); i++) {
    _vctPriIndexTask[i]._lqPriKeyStmt.Submit();
  }
}

void TableTaskMgr::CollectSecondaryTaskData(uint16_t idxPos) {}
} // namespace storage