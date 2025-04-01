#include "InsertStatement.h"
#include "../binlog/LogTask.h"
#include "../config/Configure.h"
#include "../serv/SessionPool.h"
#include "../table/TableTaskMgr.h"
#include "../utils/Log.h"

namespace storage {
StmtStatus InsertStatement::SessionExec(Session *sess) {
  // _stmtResult->SetResultStatus(ResultStatus::FINISHED);
  // _status = StmtStatus::Finished;
  // return _status;

  if (_status == StmtStatus::Created) {
    if (InitRecord())
      return StmtStatus::Finished;
  } else if (_status == StmtStatus::Executing) {
    if (_lstRecord.size() > 0) {
      for (auto iter = _lstRecord.begin(); iter != _lstRecord.end();) {
        ActionStatus s = (*iter)->_status.load(memory_order_acquire);
        if (s == ActionStatus::INIT) {
          iter++;
          continue;
        } else {
          if (s == ActionStatus::SUCEED) {
            _cntLeafRec += (*iter)->_numLeafRecord;
          }

          delete (*iter);
          iter = _lstRecord.erase(iter);
        }
      }

      if (_lstRecord.size() > 0) {
        return StmtStatus::Executing;
      }
    }

    if (_lstWaitRecord.size() > 0) {
      for (auto iter = _lstWaitRecord.begin(); iter != _lstWaitRecord.end();) {
        RecordResult res = (*iter)->GetLock()->GetRecordResult();
        if (res != RecordResult::INIT) {
          if (res == RecordResult::ERROR) {
            _stmtResult->_vctError.push_back(
                move((*iter)->GetLock()->_errMsg->GetErrorMsg()));
            _stmtFailed.store(true, memory_order_relaxed);
          }

          _lstFinishRecord.push_back(*iter);
          iter = _lstWaitRecord.erase(iter);
        } else {
          iter++;
        }
      }
    }

    if (_lstFinishRecord.size() < (size_t)_cntLeafRec) {
      return StmtStatus::Executing;
    }

    if (_stmtFailed.load(memory_order_relaxed)) {
      for (auto lr : _lstFinishRecord) {
        lr->GetLock()->_recStatus.store(RecordStatus::ROLLBACKED,
                                        memory_order_relaxed);
      }

      _stmtResult->_bFailed = true;
      _stmtResult->_rowNum = 0;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
#ifdef WITHOUT_BIN_LOG
    } else {
      for (auto lr : _lstFinishRecord) {
        lr->GetLock()->_recStatus.store(RecordStatus::COMMITED,
                                        memory_order_relaxed);
      }

      _stmtResult->_rowNum = _recordNum;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
    }
  }
#else
    } else if (sess->_transaction.IsAutoCommit()) { // Add log write queue
      _status = StmtStatus::Logging;
      LogTask::AddTransaction(ThreadPool::GetThreadId(), &(sess->_transaction));
    } else {
      _stmtResult->_rowNum = _recordNum;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Executed;
    }
  } else if (_status == StmtStatus::Logging) {
    if (sess->_transaction.IsLogged()) {
      for (auto lr : _lstFinishRecord) {
        lr->GetLock()->_recStatus.store(RecordStatus::COMMITED,
                                        memory_order_relaxed);
      }

      _stmtResult->_rowNum = _recordNum;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
    }
  }
#endif

  return _status;
}

bool InsertStatement::InitRecord() {
  ExprInsert *exprInst = GetExprInsert();
  PhysTable *table = exprInst->_exprTable->_physTable;
  TableTaskMgr *mgr = table->GetTableTaskMgr();
  IndexProp &priIndex = table->GetVectorIndex()[0];

  if (_vctParas.size() == 0) {
    _vctParas.emplace_back();
  }

  size_t para_sz = exprInst->_vctPara.size();
  MVectorPtr<MVectorPtr<ExprElem *> *> *vctRow = exprInst->_vctRowData;
  for (VectorDataValue &pvct : _vctParas) {
    if (pvct.size() != para_sz) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      SetStmtFailed(true);
      return true;
    }

    for (MVectorPtr<ExprElem *> *rowData : (*vctRow)) {
      VectorDataValue vctVal;
      priIndex._tree->CloneValues(vctVal);

      for (size_t i = 0; i < rowData->size(); i++) {
        ExprElem *elem = rowData->at(i);
        ExprColumn *col = exprInst->_vctCol->at(i);
        IDataValue *dv = ((ExprData *)elem)->Calc(pvct, vctVal);
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
      vctKey.reserve(priIndex._vctCol.size());

      for (IndexColumn &col : priIndex._vctCol) {
        IDataValue *dv = vctVal[col.colPos];
        vctKey.push_back(dv->AddRef());
      }

      StmtInsertRecord *insr =
          new StmtInsertRecord(RawKey(vctKey), move(vctVal), this, table);
      StmtInsertAction *action = new StmtInsertAction(priIndex._tree, insr);
      mgr->AddSessionAction(0, GetSessionGroupId(), action);
      _lstRecord.push_back(insr);
    }
  }

  _recordNum = (uint32_t)_lstRecord.size();
  _status = StmtStatus::Executing;
  return false;
}

} // namespace storage
