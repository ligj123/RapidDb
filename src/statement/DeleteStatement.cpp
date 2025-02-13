#include "DeleteStatement.h"

#include "../binlog/LogTask.h"
#include "../core/BranchRecord.h"
#include "../serv/SessionAction.h"
#include "../serv/SessionPool.h"
#include "../table/IndexAction.h"
#include "../table/TableTaskMgr.h"

namespace storage {

StmtStatus DeleteStatement::SessionExec(Session *sess) {
  if (_status == StmtStatus::Created) {
    assert(_midVar == nullptr);
    _midVar = new MiddleVar();

    ExprDelete *exprDel = GetExprDelete();
    size_t para_sz = exprDel->_vctPara.size();
    if (_vctPara.size() != para_sz) {
      _threadErrorMsg.reset(new ErrorMsg(EXPR_MISMATCH_COLUMN_VALUE, {}));
    }

    PhysTable *table = exprDel->_exprTable->_physTable;
    IndexSearch *idxSearch = exprDel->_exprWhere->_indexSearch;
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
          GetExprDelete()->_exprTable->_physTable->GetVectorIndex().size();
      _stmtResult->_rowNum = _totalRecNum;
      _stmtResult->SetResultStatus(ResultStatus::FINISHED);
      _status = StmtStatus::Finished;
    }
  }

  return _status;
}

bool DeleteStatement::SacnIndex(int rangePos) {
  assert(_midVar->_rangePos >= 0);
  ExprDelete *exprDel = GetExprDelete();
  PhysTable *table = exprDel->_exprTable->_physTable;
  IndexSearch *idxSearch = exprDel->_exprWhere->_indexSearch;
  int idxPos = idxSearch->_indexPos;
  assert(idxPos >= 0 && idxPos <= table->GetVectorIndex().size());

  IndexTree *idxTree = table->GetVectorIndex()[idxPos]._tree;
  IndexRange &idxRange = idxTree->GetVctRange()[_midVar->_rangePos];

  while (true) {
    KeyRange *keyRange = &_midVar->_vctKeyRange[_midVar->_keyPos];
    if (_midVar->_midPage == nullptr ||
        _midVar->_midPage->GetPageType() != PageType::LEAF_PAGE) {
      if (_midVar->_bFromRangeBegin) {
        _midVar->_midPage = idxRange._startPage;
      } else {
        if (_midVar->_midPage == nullptr) {
          _midVar->_midPage = idxRange.GetTopPage(*keyRange->_startKey);
        }

        if (!idxTree->SearchPage(*keyRange->_startKey, _midVar->_midPage)) {
          return false;
        }
      }
    }

    int pos = 0;
    LeafPage *lpage = dynamic_cast<LeafPage *>(_midVar->_midPage);
    if (!_midVar->_bFromPageBegin) {
      bool bFind;
      pos = lpage->SearchKey(*keyRange->_startKey, bFind);
      if (!keyRange->_bRange) {
        if (bFind) {
          LeafRecord *lr = &lpage->GetRecord(pos);

          if (idxPos == 0) {
            TriBool tb = HandleLeafRecord(lpage, pos, rangePos);
            if (tb == TriBool::Error) {
              SetFinished(true);
              return true;
            } else if (tb == TriBool::True) {
              _totalRecNum++;
            }
          } else {
            SendStmtRecord(idxPos, rangePos, table, this, lr, idxTree);
          }
        }

        _midVar->_keyPos++;
        if (_midVar->_keyPos < _midVar->_vctKeyRange.size()) {
          _midVar->_midPage = nullptr;
          _midVar->_bFromPageBegin = false;
          continue;
        } else {
          SetFinished(true);
          return true;
        }
      }

      if (!keyRange->_bIncLeft && bFind) {
        pos++;
      }
    }

    bool bend = false;
    for (; pos < lpage->GetRecordNumber(); pos++) {
      LeafRecord *lr = &lpage->GetRecord(pos);
      int res = lr->CompareKey(*keyRange->_endKey);
      if ((res == 0 && !keyRange->_bIncRight) || res > 0) {
        bend = true;
        break;
      }
      if (idxPos == 0) {
        TriBool tb = HandleLeafRecord(lpage, pos, rangePos);
        if (tb == TriBool::Error) {
          SetFinished(true);
          return true;
        } else if (tb == TriBool::True) {
          _totalRecNum++;
        }
      } else {
        SendStmtRecord(idxPos, rangePos, table, this, lr, idxTree);
      }
    }

    if (bend) {
      _midVar->_keyPos++;
      if (_midVar->_keyPos < _midVar->_vctKeyRange.size()) {
        _midVar->_midPage = nullptr;
        _midVar->_bFromPageBegin = false;
        if (idxRange._borderRecord->CompareKey(
                *_midVar->_vctKeyRange[_midVar->_keyPos]._startKey) < 0) {
          StatementAction *action = new StatementAction(idxTree, this);
          TableTaskMgr *mgr = table->GetTableTaskMgr();
          int rpos = idxTree->CalcIndexRange(
              *_midVar->_vctKeyRange[_midVar->_keyPos]._startKey);
          mgr->AddIndexRangeAction(idxPos, rpos, action);
        } else {
          continue;
        }
      } else {
        SetFinished(true);
        return true;
      }
    } else if (IsStmtFailed()) {
      SetFinished(true);
      return true;
    }

    _midVar->_bFromPageBegin = true;
    if (lpage->IsRangEndPage()) {
      _midVar->_midPage = nullptr;
      StatementAction *action = new StatementAction(idxTree, this);
      TableTaskMgr *mgr = table->GetTableTaskMgr();
      mgr->AddIndexRangeAction(idxPos, rangePos + 1, action);
      return true;
    } else {
      _midVar->_midPage = lpage->GetNextPage();
    }
  }

  return true;
}

TriBool DeleteStatement::HandleLeafRecord(LeafPage *page, int pagePos,
                                          int rangePos) {
  ExprDelete *exprDel = GetExprDelete();
  PhysTable *table = exprDel->_exprTable->_physTable;
  MVector<IndexProp> &vctProp = table->GetVectorIndex();
  ExprLogic *exprLogic = exprDel->_exprWhere->_exprLogic;

  LeafRecord *lr = &page->GetRecord(pagePos);
  VectorDataValue vdv;
  ReadResult res =
      lr->ReadListValue({}, vdv, vctProp[0]._tree, this, ActionType::DELETE);

  if (res != ReadResult::OK_NOLOCK) {
    _threadErrorMsg.reset(new ErrorMsg(STMT_LOCK_CONFLICT, {}));
    SessionErrMsgAction *eAction =
        new SessionErrMsgAction(this, move(_threadErrorMsg->GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), eAction);
    SetStmtFailed(true);
    return TriBool::Error;
  }

  if (exprLogic != nullptr) {
    TriBool tb = exprLogic->Calc(_vctPara, vdv);
    if (tb == TriBool::Error) {
      SessionErrMsgAction *eAction =
          new SessionErrMsgAction(this, move(_threadErrorMsg->GetErrorMsg()));
      SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), eAction);
      SetStmtFailed(true);
      return TriBool::Error;
    } else if (tb == TriBool::False) {
      return TriBool::False;
    }
  }

  VersionStamp stamp = vctProp[0]._tree->ApplyStamp(rangePos);
  LeafRecord *lrNew = lr->UpdateRecord(vctProp[0]._tree, vdv, stamp, this,
                                       ActionType::DELETE, false);
  MVector<LeafRecord *> vctLr;
  bool failed = false;

  for (size_t i = 1; i < vctProp.size(); i++) {
    IndexTree *secTree = vctProp[i]._tree;
    VectorDataValue vctKey;
    vctKey._bDecrease = false;
    vctKey.reserve(vctProp[i]._vctCol.size());

    for (IndexColumn &col : vctProp[i]._vctCol) {
      IDataValue *dv = vdv[col.colPos];
      vctKey.push_back(dv);
    }

    LeafRecord *lrSec =
        new LeafRecord(secTree, vctKey, lrNew->GetBysValue() + UI16_2_LEN,
                       lrNew->GetKeyLength(), ActionType::DELETE, stamp, this);
    vctLr.push_back(lrSec);

    if (!lrSec->IsValid()) {
      failed = true;
      break;
    }
  }

  if (failed) {
    lrNew->GetLock()->_undoRec = nullptr;
    delete lrNew;

    for (LeafRecord *lr : vctLr) {
      delete lr;
    }

    SessionErrMsgAction *eAction =
        new SessionErrMsgAction(this, move(_threadErrorMsg->GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), eAction);
    SetStmtFailed(true);
    return TriBool::Error;
  } else {
    MVector<RawRecord *> &vctRec = page->GetRecords();
    vctRec[pagePos] = lrNew;
    TableTaskMgr *mgr = table->GetTableTaskMgr();
    for (size_t i = 1; i < vctProp.size(); i++) {
      IndexTree *secTree = vctProp[i]._tree;
      RecordAction *rAction = new RecordAction(secTree, vctLr[i]);
      mgr->AddFromPrimaryAction(i, rangePos, rAction);
    }

    SessionRecordAction *action = new SessionRecordAction(this, move(vctLr));
    SessionPool::AddAction(ThreadPool::GetThreadId(), GetTxId(), action);
    return TriBool::True;
  }
}

int DeleteStatement::CalcIndexRanges(IndexTree *idxTree) {
  assert(_midVar->_keyPos >= 0 &&
         _midVar->_keyPos < _midVar->_vctKeyRange.size());
  ExprDelete *exprDel = GetExprDelete();
  PhysTable *table = exprDel->_exprTable->_physTable;
  IndexSearch *idxSearch = exprDel->_exprWhere->_indexSearch;
  int idxPos = idxSearch->_indexPos;

  assert(idxPos >= 0 && idxPos < table->GetVectorIndex().size());
  RawKey *key = _midVar->_vctKeyRange[_midVar->_keyPos]._startKey;
  return table->GetVectorIndex()[idxPos]._tree->CalcIndexRange(*key);
}

} // namespace storage
