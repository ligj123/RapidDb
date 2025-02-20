#include "TableSelectStatement.h"

#include "../binlog/LogTask.h"
#include "../core/BranchRecord.h"
#include "../result/CacheResultSet.h"
#include "../serv/SessionAction.h"
#include "../serv/SessionPool.h"
#include "../table/IndexAction.h"
#include "../table/TableTaskMgr.h"

namespace storage {

StmtStatus TableSelectStatement::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    assert(_midVar == nullptr);
    _midVar = new MiddleVar();

    ExprTableSelect *exprSel = GetExprTableSelect();
    size_t para_sz = exprSel->_vctPara.size();
    if (_vctPara.size() != para_sz) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      _stmtResult->_rowNum = 0;
      SetStmtFailed(true);
      SetFinished(true);
      return StmtStatus::Finished;
    }

    PhysTable *table = exprSel->_exprTable->_physTable;
    IndexSearch *idxSearch = exprSel->_exprWhere->_indexSearch;
    int idxPos = (idxSearch == nullptr ? 0 : idxSearch->_indexPos);
    _midVar->_table = table;
    _midVar->_indexPos = idxPos;
    IndexProp &prop = table->GetVectorIndex()[idxPos];
    _stmtResult->_resultSet = new CacheResultSet(exprSel->_vctCol);

    if (idxSearch->_bPointQuery) {
      VectorDataValue vctDv, vEmpty;
      prop._tree->CloneKeys(vctDv);
      auto vctCond = idxSearch->_vctPointCond;

      for (size_t i = 0; i < vctCond->size(); i++) {
        assert((*vctCond)[i]->GetType() == ExprType::EXPR_COMP);
        ExprComp *ecmp = dynamic_cast<ExprComp *>((*vctCond)[i]);
        assert(ecmp->_compType == CompType::EQ);
        IDataValue *dv = ecmp->_exprRight->Calc(_vctPara, vEmpty);
        vctDv[i]->Copy(*dv, true);
        dv->DecRef();
      }

      for (size_t i = vctCond->size(); i < vctDv.size(); i++) {
        vctDv[i]->SetMinValue();
      }

      RawKey *sKey = new RawKey(vctDv);
      RawKey *eKey = nullptr;
      if (vctCond->size() < vctDv.size() ||
          prop._tree->GetIndexType() == IndexType::NON_UNIQUE) {
        for (size_t i = vctCond->size(); i < vctDv.size(); i++) {
          vctDv[i]->SetMaxValue();
        }

        eKey = new RawKey(vctDv);
      }

      _midVar->_vctKeyRange.emplace_back(sKey, eKey, eKey != nullptr, true,
                                         true);
    } else {
      MVector<QueryRange> vctQR =
          ConditionConvert(idxSearch->_idxLogic, _vctPara);
      ExprField *field = GetFieldFromExprLogic(idxSearch->_idxLogic);

      for (QueryRange &range : vctQR) {
        _midVar->_vctKeyRange.push_back(
            GenIndexSearchKey(prop._tree, field, &range));
      }
    }

    StatementAction *action = new StatementAction(prop._tree, this);
    TableTaskMgr *mgr = table->GetTableTaskMgr();
    _status = StmtStatus::Executing;
    mgr->AddSessionAction(idxPos, GetSessionGroupId(), action);
  } else if (_status == StmtStatus::Executing) {
    if (_lstStmtRec.size() > 0) {
      for (auto iter = _lstStmtRec.begin(); iter != _lstStmtRec.end(); iter++) {
        ActionStatus s = (*iter)->_status.load(memory_order_acquire);
        if (s == ActionStatus::INIT) {
          iter++;
        } else {
          AddLeafRecords((*iter)->_vctLr);
          _totalRecNum += (*iter)->_numLeafRecord;
          delete (*iter);
          iter = _lstStmtRec.erase(iter);
        }
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

    if (_lstWaitRecord.size() == 0 && _lstStmtRec.size() == 0 &&
        _bFinished.load(memory_order_acquire)) {
      if (_stmtFailed.load(memory_order_relaxed)) {
        for (auto lr : _lstFinishRecord) {
          lr->GetLock()->_recStatus.store(RecordStatus::ROLLBACKED,
                                          memory_order_relaxed);
        }

        _stmtResult->_rowNum = 0;
        _stmtResult->SetResultStatus(ResultStatus::FINISHED);
        _status = StmtStatus::Finished;
      } else if (sess->_transaction.IsAutoCommit()) {
        _status = StmtStatus::Logging;
        LogTask::AddTransaction(ThreadPool::GetThreadId(),
                                &(sess->_transaction));
      } else {

        _stmtResult->_rowNum = _totalRecNum;
        _stmtResult->SetResultStatus(ResultStatus::FINISHED);
        _status = StmtStatus::Executed;
      }
    }
  } else if (_status == StmtStatus::Logging) {
    if (sess->_transaction.IsLogged()) {
      for (auto lr : _lstFinishRecord) {
        lr->GetLock()->_recStatus.store(RecordStatus::COMMITED,
                                        memory_order_relaxed);
      }

      sess->_transaction.SetTranStatus(TranStatus::FINISHED);
      size_t idxNum =
          GetExprTableSelect()->_exprTable->_physTable->GetVectorIndex().size();
      _stmtResult->_rowNum = _totalRecNum;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
    }
  }

  return _status;
}

TriBool TableSelectStatement::HandleLeafRecord(LeafPage *page, int pagePos,
                                               int rangePos,
                                               VectorLeafRecord *vctLeafRec) {
  ExprTableSelect *exprSel = GetExprTableSelect();
  PhysTable *table = exprSel->_exprTable->_physTable;
  MVector<IndexProp> &vctProp = table->GetVectorIndex();
  ExprLogic *exprLogic = exprSel->_exprWhere->_exprLogic;
  ActionType aType = ActionType::NO_ACTION;
  if (exprSel->_lockType == LockType::SHARE_LOCK) {
    aType = ActionType::READ_SHARE;
  } else if (exprSel->_lockType == LockType::WRITE_LOCK) {
    aType = ActionType::READ_UPDATE;
  }

  LeafRecord *lr = &page->GetRecord(pagePos);
  VectorDataValue vdv;
  ReadResult res = lr->ReadListValue({}, vdv, vctProp[0]._tree, this, aType);

  if (res != ReadResult::OK_NOLOCK && res != ReadResult::OK_LOCK) {
    _threadErrorMsg.reset(new ErrorMsg(STMT_LOCK_CONFLICT, {}));
    SendErrMsg(move(_threadErrorMsg->GetErrorMsg()));
    return TriBool::Error;
  }

  if (exprLogic != nullptr) {
    TriBool tb = exprLogic->Calc(_vctPara, vdv);
    if (tb == TriBool::Error) {
      SendErrMsg(move(_threadErrorMsg->GetErrorMsg()));
      lr->SubmitStatement(*this, RecordStatus::FREEED);
      return TriBool::Error;
    } else if (tb == TriBool::False) {
      lr->SubmitStatement(*this, RecordStatus::FREEED);
      return TriBool::False;
    }
  }

  VectorDataValue vctDv;
  vctDv.reserve(exprSel->_vctCol->size());
  for (ExprColumn *ecol : *exprSel->_vctCol) {
    IDataValue *dv = vdv[ecol->_pos];
    if (res == ReadResult::OK_LOCK || !dv->IsArrayType()) {
      vctDv.push_back(dv->AddRef());
    } else
      vctDv.push_back(dv->Clone(true));
  }

  if (_midVar->_indexPos == 0) {
    _stmtResult->_resultSet->AddRow(move(vctDv));
    if (res == ReadResult::OK_LOCK) {

      AddLeafRecord(lr);
    }
  } else {
    assert(vctLeafRec != nullptr);
    vctLeafRec->push_back(lr);
  }

  return TriBool::True;
}

} // namespace storage