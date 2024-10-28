#include "TableTaskMgr.h"

#include "IndexTask.h"

namespace storage {

inline uint16_t JudgePriKeyRange(const RawKey &key,
                                 const MVector<LeafRecord> &vctRecBorder) {

  for (size_t i = 0; i < vctRecBorder.size(); i++) {
    LeafRecord &lr = _vctRecBorder[i];
    if (lr.CompareKey(key) <= 0) {
      return i;
    }
  }

  return _vctRecBorder.size();
}

inline uint16_t JudgeLeafRecordRange(const LeafRecord &lr,
                                     const MVector<LeafRecord> &vctRecBorder) {
  for (size_t i = 0; i < vctRecBorder.size(); i++) {
    LeafRecord &blr = vctRecBorder[i];
    if (blr.CompareTo(lr) <= 0) {
      return i;
    }
  }

  return vctRecBorder.size();
}

void TableTaskMgr::CollectPrimaryTaskData() {
  if (!_priIndexTaskQueue._spinMutex.try_lock()) {
    return;
  }

  MDeque<Statement *> qs;
  _priIndexTaskQueue._fqStmt.Pop(qs);

  for (auto iter = qs.begin(); iter != qs.end(); iter++) {
    Statement *stmt = *iter;
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

void TableTaskMgr::CollectSecondaryTaskData(uint16_t idxPos) {
  assert(IndexPage < _vctSecIndexTaskQueue.size());
  SecIndexTaskQueue &sitq = _vctSecIndexTaskQueue[idxPos];
  if (!sitq._spinMutex.try_lock()) {
    return;
  }

  MVector<SecIndexTask> &vsec = _vctSecIndexTasks[idxPos];
  MDeque<Statement *> qs;
  sitq._fqStmt.Pop(qs);

  for (auto iter = qs.begin(); iter != qs.end(); iter++) {
    Statement *stmt = *iter;

    MVector<uint16_t> vs = stmt->SpliteRange(sitq._vctRecBorder);
    for (uint16_t pos : vs) {
      vsec[pos]._lqStmt.Push(stmt);
    }
  }

  for (size_t i = 0; i < vsec.size(); i++) {
    vsec[pos]._lqStmt.Submit();
  }

  MDeque<LeafRecord *> ql;
  sitq._fqRec.Pop(ql);

  for (auto iter = ql.begin(); iter != ql.end(); iter++) {
    LeafRecord *lr = *iter;
    uint16_t pos = JudgeLeafRecordRange(*lr, sitq._vctRecBorder);
    vsec[pos]._lqRecord.Push(lr, false);
  }

  for (size_t i = 0; i < vsec.size(); i++) {
    vsec[pos]._lqRecord.Submit();
  }
}
} // namespace storage