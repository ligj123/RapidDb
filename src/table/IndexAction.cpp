#include "IndexAction.h"

#include "../core/BranchPage.h"
#include "../core/BranchRecord.h"
#include "../core/IndexTree.h"
#include "../core/LeafPage.h"
#include "../core/LeafRecord.h"
#include "../serv/SessionPool.h"
#include "../statement/InsertStatement.h"
#include "../statement/Statement.h"
#include "../utils/ThreadPool.h"
#include "Table.h"
#include "TableTaskMgr.h"

namespace storage {
int PrevPageAction::JudgeRange() {
  if (_page == nullptr) {
    _page = (LeafPage *)_indexTree->GetPage(_pageId, PageType::LEAF_PAGE,
                                            nullptr, true);
  }

  if (_page->GetRecordNumber() > 0) {
    LeafRecord &lr = _page->GetRecord(0);
    _rangePos = _indexTree->CalcIndexRange(lr);
  } else {
    BranchPage *parent = _page->GetParentPage();
    if (parent == nullptr) {
      parent = (BranchPage *)_indexTree->GetPage(
          _page->GetParentPageId(), PageType::BRANCH_PAGE, nullptr, true);
    }

    BranchRecord &br = parent->GetRecord(0, false);
    _rangePos = _indexTree->CalcIndexRange(br);
  }

  return _rangePos;
}

TaskStatus PrevPageAction::Exec() {
  assert(_rangePos >= 0 && _rangePos < _indexTree->GetVctRange().size());
  if (_page == nullptr) {
    _page = (LeafPage *)_indexTree->GetPage(_pageId, PageType::LEAF_PAGE,
                                            nullptr, true);
  }

  PageStatus s = _page->GetPageStatus();
  if (s == PageStatus::READING) {
    _page->SetPageStatus(PageStatus::VALID, true);
  } else if (s != PageStatus::VALID) {
    return TaskStatus::INTERVAL;
  }

  assert(_pageId == _page->GetPageId());
  _page->SetPrevPageId(_prevPageId);
  _page->AddWriteQueue(_indexTree->GetVctRange()[_rangePos]._pageMap);
  return TaskStatus::FINISHED;
}

TaskStatus RecordAction::Exec() {
  RecordLock *lock = _lr->GetLock();
  if (lock->GetRecordStatus() != RecordStatus::INIT) {
    assert(lock->GetRecordResult() != RecordResult::IN_PAGE &&
           lock->GetRecordStatus() == RecordStatus::ROLLBACKED);
    delete _lr;
    _lr = nullptr;
    return TaskStatus::FINISHED;
  }

  if (_idxPage == nullptr) {
    _idxPage = _indexTree->GetRootPage();
  }
  if (_idxPage->GetPageType() != PageType::LEAF_PAGE) {
    bool b = _indexTree->SearchPage(*_lr, _idxPage);
    if (!b) {
      return TaskStatus::INTERVAL;
    }
  }

  LeafPage *lp = (LeafPage *)_idxPage;
  lp->UpdateAction(_lr);
  if (_lr->GetLock()->_recResult == RecordResult::ERROR) {
    _indexTree->GetVctRange()[_rangePos]._vctErrRecord.push_back(_lr);
  } else {
    lp->AddWriteQueue(_indexTree->GetVctRange()[_rangePos]._pageMap);
    if (lp->NeedForceSplit()) {
      lp->SplitPage(_indexTree->GetVctRange()[_rangePos]._pageMap);
    }
  }

  _lr = nullptr;
  return TaskStatus::FINISHED;
}

int RecordAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(*_lr);
  return {_rangePos};
}

TaskStatus StatementAction::Exec() {
  bool b;
  if (_indexTree->GetIndexType() == IndexType::PRIMARY) {
    b = _stmt->PrimaryKeyExec(_rangePos);
  } else {
    b = _stmt->SecondaryKeyExec(_rangePos);
  }

  return b ? TaskStatus::FINISHED : TaskStatus::INTERVAL;
}

int StatementAction::JudgeRange() { return _stmt->CalcIndexRanges(_indexTree); }

TaskStatus StmtInsertAction::Exec() {
  if (_stmtRecord->_stmt->IsStmtFailed()) {
    _stmtRecord->_status.store(ActionStatus::FAILED, memory_order_release);
    return TaskStatus::FINISHED;
  }

  MVector<storage::IndexProp> &vctProp = _stmtRecord->_table->GetVectorIndex();
  MVector<LeafRecord *> vctLr;

  VersionStamp stamp = vctProp[0]._tree->ApplyStamp(_rangePos);
  LeafRecord *lrPri =
      new LeafRecord(vctProp[0]._tree, _stmtRecord->_priKey,
                     _stmtRecord->_vctParas, stamp, _stmtRecord->_stmt);

  if (!lrPri->IsValid()) {
    SessionErrMsgAction *eAction = new SessionErrMsgAction(
        _stmtRecord->_stmt, move(_threadErrorMsg->GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(),
                           _stmtRecord->_stmt->GetTxId(), eAction);
    _stmtRecord->_stmt->SetStmtFailed(true);
    _stmtRecord->_status.store(ActionStatus::FAILED, memory_order_relaxed);
    delete lrPri;
    return TaskStatus::FINISHED;
  }

  vctLr.push_back(lrPri);
  bool failed = false;

  for (size_t i = 1; i < vctProp.size(); i++) {
    IndexTree *secTree = vctProp[i]._tree;
    VectorDataValue vctKey;
    vctKey._bDecrease = false;
    vctKey.reserve(vctProp[i]._vctCol.size());

    for (IndexColumn &col : vctProp[i]._vctCol) {
      IDataValue *dv = _stmtRecord->_vctParas[col.colPos];
      vctKey.push_back(dv);
    }

    LeafRecord *lrSec = new LeafRecord(
        secTree, vctKey, lrPri->GetBysValue() + UI16_2_LEN,
        lrPri->GetKeyLength(), ActionType::INSERT, stamp, _stmtRecord->_stmt);
    vctLr.push_back(lrSec);

    if (!lrSec->IsValid()) {
      failed = true;
      break;
    }
  }

  if (failed) {
    for (LeafRecord *lr : vctLr) {
      delete lr;
    }

    vctLr.clear();
    SessionErrMsgAction *eAction = new SessionErrMsgAction(
        _stmtRecord->_stmt, move(_threadErrorMsg->GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(),
                           _stmtRecord->_stmt->GetTxId(), eAction);
    _stmtRecord->_stmt->SetStmtFailed(true);
    _stmtRecord->_status.store(ActionStatus::FAILED, memory_order_relaxed);
  } else {
    RecordAction *pAction =
        new RecordAction(vctProp[0]._tree, vctLr[0], _rangePos);
    vctProp[0]._tree->AddActionFromLocal(_rangePos, pAction);
    TableTaskMgr *mgr = _stmtRecord->_table->GetTableTaskMgr();

    for (size_t i = 1; i < vctProp.size(); i++) {
      IndexTree *secTree = vctProp[i]._tree;
      RecordAction *rAction = new RecordAction(secTree, vctLr[i]);
      mgr->AddFromPrimaryAction(i, _rangePos, rAction);
    }

    _stmtRecord->_numLeafRecord = vctLr.size();
    SessionRecordAction *action =
        new SessionRecordAction(_stmtRecord->_stmt, move(vctLr));
    SessionPool::AddAction(ThreadPool::GetThreadId(),
                           _stmtRecord->_stmt->GetTxId(), action);
    _stmtRecord->_status.store(ActionStatus::SUCEED, memory_order_relaxed);
  }

  return TaskStatus::FINISHED;
}

int StmtInsertAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_stmtRecord->_priKey);
  return _rangePos;
}

TaskStatus StmtSecRecordAction::Exec() {
  if (_stmtSecRec->_stmt->IsStmtFailed()) {
    _stmtSecRec->_status.store(ActionStatus::FAILED, memory_order_relaxed);
    return TaskStatus::FINISHED;
  }

  if (_idxPage == nullptr) {
    _idxPage = _indexTree->GetRootPage();
  }

  const RawKey priKey = _stmtSecRec->_secLr->GetPrimayKey();
  if (_idxPage->GetPageType() != PageType::LEAF_PAGE) {
    bool b = _indexTree->SearchPage(priKey, _idxPage);
    if (!b) {
      return TaskStatus::INTERVAL;
    }
  } else {
    PageStatus s = _idxPage->GetPageStatus();
    if (s != PageStatus::VALID && s != PageStatus::WRITING) {
      if (s == PageStatus::READING) {
        return TaskStatus::INTERVAL;
      } else if (s == PageStatus::READED) {
        _idxPage->SetPageStatus(PageStatus::VALID, true);
      }
    }
  }

  LeafPage *lp = dynamic_cast<LeafPage *>(_idxPage);
  bool bFind = false;
  int32_t pos = lp->SearchKey(priKey, bFind);
  assert(bFind);

  VectorDataValue vct;
  LeafRecord &lr = lp->GetRecord(pos);
  ReadResult rr = lr.ReadListValue({}, vct, _indexTree, _stmtSecRec->_stmt,
                                   ActionType::NO_ACTION);
  assert(rr = ReadResult::OK_NOLOCK);

  ExprType type = _stmtSecRec->_stmt->GetType();
  ExprLogic *exprLogic = nullptr;
  if (type == ExprType::EXPR_UPDATE) {
    exprLogic =
        dynamic_cast<ExprUpdate *>(_stmtSecRec->_stmt->GetExprStatement())
            ->_exprWhere->_exprLogic;
  } else {
    assert(type == ExprType::EXPR_DELETE);
    exprLogic =
        dynamic_cast<ExprDelete *>(_stmtSecRec->_stmt->GetExprStatement())
            ->_exprWhere->_exprLogic;
  }

  if (exprLogic != nullptr) {
    TriBool tb = exprLogic->Calc(_stmtSecRec->_stmt->GetParameters(), vct);
    if (tb == TriBool::Error) {
      SessionErrMsgAction *eAction = new SessionErrMsgAction(
          _stmtSecRec->_stmt, move(_threadErrorMsg->GetErrorMsg()));
      SessionPool::AddAction(ThreadPool::GetThreadId(),
                             _stmtSecRec->_stmt->GetTxId(), eAction);
      _stmtSecRec->_stmt->SetStmtFailed(true);
      _stmtSecRec->_status.store(ActionStatus::FAILED, memory_order_relaxed);
      return TaskStatus::FINISHED;
    } else if (tb == TriBool::False) {
      _stmtSecRec->_status.store(ActionStatus::SUCEED, memory_order_relaxed);
      return TaskStatus::FINISHED;
    }
  }

  if (!lr.UpdateAble(_stmtSecRec->_stmt->GetTxId())) {
    ErrorMsg msg(TRAN_LOCK_CONFLICT, {});
    SessionErrMsgAction *eAction =
        new SessionErrMsgAction(_stmtSecRec->_stmt, move(msg.GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(),
                           _stmtSecRec->_stmt->GetTxId(), eAction);
    _stmtSecRec->_stmt->SetStmtFailed(true);
    _stmtSecRec->_status.store(ActionStatus::FAILED, memory_order_relaxed);
    return TaskStatus::FINISHED;
  }

  MVector<LeafRecord *> vctLr;
  MVector<storage::IndexProp> &vctProp = _stmtSecRec->_table->GetVectorIndex();
  bool bFailed = false;

  if (type == ExprType::EXPR_UPDATE) {
    MVectorPtr<storage::ExprColumn *> *vctCol =
        dynamic_cast<ExprUpdate *>(_stmtSecRec->_stmt->GetExprStatement())
            ->_vctCol;
    for (size_t i = 0; i < vctCol->size(); i++) {
      ExprColumn *ecol = vctCol->at(i);
      ExprData *edata = dynamic_cast<ExprData *>(ecol->_exprElem);
      IDataValue *dv = edata->Calc(_stmtSecRec->_stmt->GetParameters(), vct);
      vct[ecol->_pos]->DecRef();
      vct[ecol->_pos] = dv;
    }

    VersionStamp stamp = vctProp[0]._tree->ApplyStamp(_rangePos);
    lr.UpdateRecord(vctProp[0]._tree, vct, stamp, _stmtSecRec->_stmt,
                    ActionType::UPDATE, false);
    for (size_t i = 1; i < vctProp.size(); i++) {
      IndexProp &prop = vctProp[i];
      VectorDataValue vctSec = prop.GenSecondaryData(vct);
      LeafRecord *lrSec = new LeafRecord(
          prop._tree, vctSec, lr.GetBysValue() + UI16_2_LEN, lr.GetKeyLength(),
          ActionType::UPDATE, stamp, _stmtSecRec->_stmt);
      vctLr.push_back(lrSec);
      if (!lrSec->IsValid()) {
        bFailed = true;
        break;
      }
    }
  } else {
    VersionStamp stamp = vctProp[0]._tree->ApplyStamp(_rangePos);
    lr.UpdateRecord(vctProp[0]._tree, vct, stamp, _stmtSecRec->_stmt,
                    ActionType::DELETE, false);

    for (size_t i = 1; i < vctProp.size(); i++) {
      IndexProp &prop = vctProp[i];
      VectorDataValue vctSec = prop.GenSecondaryData(vct);
      LeafRecord *lrSec = new LeafRecord(
          prop._tree, vctSec, lr.GetBysValue() + UI16_2_LEN, lr.GetKeyLength(),
          ActionType::DELETE, stamp, _stmtSecRec->_stmt);
      vctLr.push_back(lrSec);
      if (!lrSec->IsValid()) {
        bFailed = true;
        break;
      }
    }
  }

  if (bFailed) {
    lr.SubmitStatement(*_stmtSecRec->_stmt, RecordStatus::ROLLBACKED);
    lr.ReleaseLock(vctProp[0]._tree);

    for (LeafRecord *lrSec : vctLr) {
      delete lrSec;
    }

    SessionErrMsgAction *eAction = new SessionErrMsgAction(
        _stmtSecRec->_stmt, move(_threadErrorMsg->GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(),
                           _stmtSecRec->_stmt->GetTxId(), eAction);
    _stmtSecRec->_stmt->SetStmtFailed(true);
    _stmtSecRec->_status.store(ActionStatus::FAILED, memory_order_relaxed);
    return TaskStatus::FINISHED;
  }

  TableTaskMgr *mgr = _stmtSecRec->_table->GetTableTaskMgr();

  for (size_t i = 1; i < vctProp.size(); i++) {
    IndexTree *secTree = vctProp[i]._tree;
    RecordAction *rAction = new RecordAction(secTree, vctLr[i - 1]);
    mgr->AddFromPrimaryAction(i, _rangePos, rAction);
  }

  vctLr.push_back(&lr);
  _stmtSecRec->_numLeafRecord = vctLr.size();
  SessionRecordAction *action =
      new SessionRecordAction(_stmtSecRec->_stmt, move(vctLr));
  SessionPool::AddAction(ThreadPool::GetThreadId(),
                         _stmtSecRec->_stmt->GetTxId(), action);
  _stmtSecRec->_status.store(ActionStatus::SUCEED, memory_order_relaxed);

  return TaskStatus::FINISHED;
}

int StmtSecRecordAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_stmtSecRec->_secLr->GetPrimayKey());
  return _rangePos;
}

} // namespace storage