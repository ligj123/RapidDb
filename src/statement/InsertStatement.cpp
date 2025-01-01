#include "InsertStatement.h"
#include "../config/Configure.h"
#include "../serv/SessionPool.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

namespace storage {
StmtStatus InsertStatement::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    if (InitRecord())
      return StmtStatus::Executing;
  } else if (_status == StmtStatus::Executing) {
    if (_lstRecord.size() > 0) {
      uint32_t idxSz =
          (uint32_t)_exprInsert->_physTable->GetVectorIndex().size();
      for (auto iter = _lstRecord.begin(); iter != _lstRecord.end();) {
        if ((*iter)->_status == ActionStatus::INIT) {
          iter++;
          continue;
        }

        if ((*iter)->_status == ActionStatus::SUCEED) {
          _recordCount += idxSz;
        }

        iter = _lstRecord.erase(iter);
      }

      if (_lstRecord.size() > 0) {
        return StmtStatus::Executing;
      }
    }

    if (_lstWaitRecord.size() > 0) {
      for (auto iter = _lstWaitRecord.begin(); iter != _lstWaitRecord.end();) {
        if ((*iter)->GetLock()->GetRecordResult() != RecordResult::INIT) {
          _lstFinshRecord.push_back(*iter);
          iter = _lstWaitRecord.erase(iter);
        } else {
          iter++;
        }
      }
    }

    if (_lstFinshRecord.size() < (size_t)_recordCount) {
      return StmtStatus::Executing;
    }

    if (_stmtFailed.load(memory_order_relaxed)) {
      for (auto lr : _lstFinshRecord) {
        lr->GetLock()->_recStatus.store(RecordStatus::ROLLBACKED,
                                        memory_order_relaxed);
      }

      _stmtResult->_rowNum = 0;
      _status = StmtStatus::Finished;
    } else if (sess->_transaction.IsAutoCommit()) { // Add log write queue

      _status = StmtStatus::Logging;
      // Add log write queue
    } else {
      _status = StmtStatus::Executed;
    }
  } else if (_status == StmtStatus::Logged) {
    _status = StmtStatus::Finished;
  }

  return _status;
}

bool InsertStatement::InitRecord() {
  PhysTable *table = _exprInsert->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  IndexProp &priIndex = table->GetVectorIndex()[0];

  if (_vctParas.size() == 0) {
    _vctParas.push_back(new VectorDataValue());
  }

  MVectorPtr<MVectorPtr<ExprElem *> *> *vctRow = _exprInsert->_vctRowData;
  for (VectorDataValue *pvct : _vctParas) {
    for (MVectorPtr<ExprElem *> *rowData : (*vctRow)) {
      VectorDataValue vctVal;
      priIndex._tree->CloneValues(vctVal);

      for (size_t i = 0; i < rowData->size(); i++) {
        ExprElem *elem = rowData->at(i);
        ExprColumn *col = _exprInsert->_vctCol->at(i);
        IDataValue *dv = ((ExprData *)elem)->Calc(*pvct, vctVal);
        bool b = vctVal[col->_pos]->Copy(*dv, true);
        dv->DecRef();

        if (!b) {
          _stmtResult->_vctError.push_back(
              move(_threadErrorMsg->GetErrorMsg()));
          SetStmtFailed(true);
          _status = StmtStatus::Executed;
          return true;
        }
      }

      if (!table->CheckColumnValues(vctVal)) {
        _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
        SetStmtFailed(true);
        _status = StmtStatus::Executed;
        return true;
      }

      VectorDataValue vctKey;
      vctKey._bDecrease = false;
      vctKey.reserve(priIndex._vctCol.size());

      for (IndexColumn &col : priIndex._vctCol) {
        IDataValue *dv = vctVal[col.colPos];
        vctKey.push_back(dv);
      }

      StmtInsertRecord *insr =
          new StmtInsertRecord(RawKey(vctKey), move(vctVal), this, table);
      StmtInsertAction *action = new StmtInsertAction(priIndex._tree, insr);
      mgr->AddSessionAction(0, GetSessionId(), action);
      _lstRecord.push_back(insr);
    }
  }

  _status = StmtStatus::Executing;
  return false;
}

StmtStatus InsertStatement::CheckStatus() { return _status; }

void InsertStatement::CollectLogRecords(
    MTreeSet<LeafRecord *, LeafRecordCmp> &setRec) {
  if (_stmtFailed.load(memory_order_relaxed)) {
    return;
  }

  for (LeafRecord *lr : _lstFinshRecord) {
    assert(lr->GetLock()->_recResult != RecordResult::INIT);
    if (lr->GetLock()->_recResult == RecordResult::ERROR) {
      continue;
    }

    setRec.insert(lr);
  }
}

void InsertStatement::Commit() {
  if (_stmtFailed.load(memory_order_relaxed)) {
    return;
  }

  for (LeafRecord *lr : _lstFinshRecord) {
    lr->SubmitStatement(*this, RecordStatus::COMMITED);
  }
}

void InsertStatement::Rollback() {
  if (_stmtFailed.load(memory_order_relaxed)) {
    return;
  }

  for (LeafRecord *lr : _lstFinshRecord) {
    lr->SubmitStatement(*this, RecordStatus::ROLLBACKED);
  }
}
} // namespace storage
