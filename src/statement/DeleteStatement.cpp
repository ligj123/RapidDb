#include "DeleteStatement.h"

#include "../table/TableTaskMgr.h"

namespace storage {

StmtStatus DeleteStatement::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    assert(_rangePos == -1);
    ExprDelete *exprDel = GetExprDelete();
    PhysTable *table = exprDel->_exprTable->_physTable;
    int idxPos = exprDel->_exprWhere->_useIndex->_indexPos;
    MVector<IndexProp> &vctProp = table->GetVectorIndex();
    assert(idxPos > 0 && idxPos < vctProp.size());

    int rpos = CalcIndexRanges(vctProp[idxPos]._tree);
    StatementAction *action = new StatementAction(vctProp[idxPos]._tree, this);
    TableTaskMgr *mgr = table->GetTableTaskMgr();
    _status = StmtStatus::Executing;
    mgr->AddSessionAction(idxPos, GetSessionGroupId(), action);
  } else if (_status == StmtStatus::Executing) {
    if (_lstStmtRec.size() > 0) {
      for (auto iter = _lstStmtRec.begin(); iter != _lstStmtRec.end(); iter++) {
        ActionStatus s = (*iter)->_status.load(memory_order_acquire);
        if (s == ActionStatus::INIT) {
          iter++;
          continue;
        } else {
          if (s == ActionStatus::SUCEED) {
            _totalRecNum += (*iter)->_numLeafRecord;
          }

          delete (*iter);
          iter = _lstStmtRec.erase(iter);
        }
      }

      if (_lstStmtRec.size() == 0) {
        _bFinished = true;
      }
    }

    if (_lstWaitRecord.size() > 0) {
      for (auto iter = _lstWaitRecord.begin(); iter != _lstWaitRecord.end();) {
        if ((*iter)->GetLock()->GetRecordResult() != RecordResult::INIT) {
          _lstFinishRecord.push_back(*iter);
          iter = _lstWaitRecord.erase(iter);
        } else {
          iter++;
        }
      }
    }
  }

  return _status;
}

bool DeleteStatement::PrimaryKeyExec(int rangePos) { return false; }

bool DeleteStatement::SecondaryKeyExec(int rangePos) { return false; }

int DeleteStatement::CalcIndexRanges(IndexTree *idxTree) { return -1; }

void DeleteStatement::CollectLogRecords(TreeSetRecord &setRec) {}

void DeleteStatement::Commit() {}

void DeleteStatement::Rollback() {}

} // namespace storage
