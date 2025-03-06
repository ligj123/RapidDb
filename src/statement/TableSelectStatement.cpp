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
    assert(_stmtResult->_resultSet == nullptr);
    _midVar = new MiddleVar();
    ExprTableSelect *exprSel = GetExprTableSelect();

    size_t para_sz = exprSel->_vctPara.size();
    if (_vctPara.size() != para_sz) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
      _stmtResult->_vctError.push_back(move(_threadErrorMsg->GetErrorMsg()));
      _stmtResult->_rowNum = 0;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
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
        if (dv == nullptr || !vctDv[i]->Copy(*dv, true)) {
          if (dv != nullptr) {
            dv->DecRef();
          }

          _stmtResult->_vctError.push_back(
              move(_threadErrorMsg->GetErrorMsg()));
          _stmtResult->_rowNum = 0;
          SetStmtFailed(true);
          SetFinished(true);
          return StmtStatus::Finished;
        } else {
          dv->DecRef();
        }
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
    if (!_bFinished.load(memory_order_acquire)) {
      return _status;
    }

    for (auto iter = _lstStmtRec.begin(); iter != _lstStmtRec.end();) {
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

    if (_lstStmtRec.size() > 0) {
      return _status;
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

    if (_lstWaitRecord.size() == 0 && _lstStmtRec.size() == 0) {
      if (_stmtFailed.load(memory_order_relaxed)) {
        for (auto lr : _lstFinishRecord) {
          lr->GetLock()->_recStatus.store(RecordStatus::ROLLBACKED,
                                          memory_order_relaxed);
        }

        _stmtResult->_bFailed = true;
        _stmtResult->_rowNum = 0;
        _stmtResult->SetResultStatus(ResultStatus::FINISHED);
        _status = StmtStatus::Finished;
      } else if (sess->_transaction.IsAutoCommit()) {
        for (auto lr : _lstFinishRecord) {
          lr->GetLock()->_recStatus.store(RecordStatus::FREEED,
                                          memory_order_relaxed);
        }

        _status = StmtStatus::Finished;
      } else {
        _status = StmtStatus::Executed;
      }

      _stmtResult->_rowNum = _stmtResult->_resultSet->GetRowCount();
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
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
  if (lr->ReleaseLockAble()) {
    int32_t commLen1, commLen2, tempLen1, tempLen2;
    lr->GetLength(tempLen1, commLen1);
    lr->ReleaseLock(page->GetIndexTree());
    lr->GetLength(tempLen2, commLen2);
    page->UpdateDataLength(commLen2 - commLen1, tempLen2 - tempLen1);
  }

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
      if (res == ReadResult::OK_LOCK) {
        lr->SubmitStatement(*this, RecordStatus::FREEED);
      }
      return TriBool::Error;
    } else if (tb == TriBool::False) {
      if (res == ReadResult::OK_LOCK) {
        lr->SubmitStatement(*this, RecordStatus::FREEED);
      }
      return TriBool::False;
    }
  }

  VectorDataValue vctDv;
  vctDv.reserve(exprSel->_vctCol->size());
  for (ExprColumn *ecol : *exprSel->_vctCol) {
    IDataValue *dv = vdv[ecol->_pos];
    if (res == ReadResult::OK_LOCK || !dv->IsArrayType()) {
      vctDv.push_back(dv->AddRef());
    } else {
      vctDv.push_back(dv->Clone(true));
    }
  }

  if (_midVar->_indexPos == 0) {
    if (res == ReadResult::OK_LOCK) {
      AddLeafRecord(lr);
    }

    _totalRecNum++;
  } else {
    assert(vctLeafRec != nullptr);
    if (res == ReadResult::OK_LOCK) {
      vctLeafRec->push_back(lr);
    }
  }

  _stmtResult->_resultSet->AddRow(move(vctDv));
  return TriBool::True;
}

} // namespace storage