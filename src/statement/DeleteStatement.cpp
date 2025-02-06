#include "DeleteStatement.h"

#include "../binlog/LogTask.h"
#include "../table/TableTaskMgr.h"

namespace storage {

StmtStatus DeleteStatement::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    assert(_rangePos == -1);
    ExprDelete *exprDel = GetExprDelete();
    PhysTable *table = exprDel->_exprTable->_physTable;

    IndexSearch *idxSearch = exprDel->_exprWhere->_indexSearch;
    int idxPos = idxSearch->_indexPos;

    _indexCondition._vctValue =
        ConditionConvert(idxSearch->_idxLogic, _vctPara);
    _indexCondition._field = GetFrieldFromExprLogic(idxSearch->_idxLogic);

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
        } else {
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
        size_t idxNum =
            GetExprDelete()->_exprTable->_physTable->GetVectorIndex().size();
        _stmtResult->_rowNum = _lstFinishRecord.size() / idxNum;
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
          GetExprDelete()->_exprTable->_physTable->GetVectorIndex().size();
      _stmtResult->_rowNum = _lstFinishRecord.size() / idxNum;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
    }
  }

  return _status;
}

bool DeleteStatement::PrimaryKeyExec() { return false; }

bool DeleteStatement::SecondaryKeyExec() {
  assert(_rangePos >= 0);
  ExprDelete *exprDel = GetExprDelete();
  PhysTable *table = exprDel->_exprTable->_physTable;
  IndexSearch *idxSearch = exprDel->_exprWhere->_indexSearch;
  int idxPos = idxSearch->_indexPos;
  assert(idxPos >= 0 && idxPos <= table->GetVectorIndex().size());

  IndexTree *idxTree = table->GetVectorIndex()[idxPos]._tree;
  BranchPage *parentPage = nullptr;
  LeafPage *leafPage = nullptr;

  if (_bFromBegin) {
    IndexRange &range = idxTree->GetVctRange()[_rangePos];
    leafPage = range._startPage;
    parentPage = leafPage->GetParentPage();
  } else {
  }

  return false;
}

int DeleteStatement::CalcIndexRanges(IndexTree *idxTree) { return -1; }

void DeleteStatement::CollectLogRecords(TreeSetRecord &setRec) {}

void DeleteStatement::Commit() {}

void DeleteStatement::Rollback() {}

} // namespace storage
