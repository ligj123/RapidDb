#include "SelectStatement.h"

#include "../binlog/LogTask.h"
#include "../core/BranchRecord.h"
#include "../serv/SessionAction.h"
#include "../serv/SessionPool.h"
#include "../table/IndexAction.h"
#include "../table/TableTaskMgr.h"

namespace storage {

StmtStatus SelectStatement::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    assert(_midVar == nullptr);
    _midVar = new MiddleVar();

    ExprTableSelect *exprSel = GetExprTableSelect();
    size_t para_sz = exprSel->_vctPara.size();
    if (_vctPara.size() != para_sz) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
    }

    PhysTable *table = exprSel->_exprTable->_physTable;
    IndexSearch *idxSearch = exprSel->_exprWhere->_indexSearch;
    int idxPos = idxSearch->_indexPos;

    if (idxSearch->_bPointQuery) {
      IndexProp &prop = table->GetVectorIndex()[idxPos];
      VectorDataValue vctDv, vEmpty;
      prop._tree->CloneKeys(vctDv);
      auto vctCond = idxSearch->_vctPointCond;

      for (size_t i = 0; i < vctCond->size(); i++) {
        assert((*vctCond)[i]->GetType() == ExprType::EXPR_COMP);
        ExprComp *ecmp = dynamic_cast<ExprComp *>((*vctCond)[i]);
        IDataValue *dv = ecmp->_exprRight->Calc(_vctPara, vEmpty);
        vctDv[i]->Copy(*dv, true);
        dv->DecRef();
      }

      for (size_t i = vctCond->size(); i < vctDv.size(); i++) {
        vctDv[i]->SetMinValue();
      }

      RawKey *sKey = new RawKey(vctDv);
      RawKey *eKey = nullptr;
      if (vctCond->size() < vctDv.size()) {
        for (size_t i = vctCond->size(); i < vctDv.size(); i++) {
          vctDv[i]->SetMaxValue();
        }

        eKey = new RawKey(vctDv);
      }

      _midVar->_vctKeyRange.emplace_back(sKey, eKey, eKey != nullptr, true,
                                         true);
    } else {
      IndexProp &prop = table->GetVectorIndex()[idxPos];
      MVector<QueryRange> vctQR =
          ConditionConvert(idxSearch->_idxLogic, _vctPara);
      ExprField *field = GetFrieldFromExprLogic(idxSearch->_idxLogic);

      for (QueryRange &range : vctQR) {
        _midVar->_vctKeyRange.push_back(
            GenIndexSearchKey(prop._tree, field, &range));
      }
    }

    MVector<IndexProp> &vctProp = table->GetVectorIndex();

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
        } else {
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

    if (_lstWaitRecord.size() == 0 && _lstStmtRec.size() == 0 && _bFinished) {
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

TriBool SelectStatement::HandleLeafRecord(LeafPage *page, int pagePos,
                                          int rangePos) {
  return TriBool::Error;
}

} // namespace storage