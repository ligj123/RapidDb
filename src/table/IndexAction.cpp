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
    _indexTree->GetVctRange()[_rangePos]._lstErrRecord.push_back(_lr);
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
  return _rangePos;
}

TaskStatus StatementAction::Exec() {
  bool b = _stmt->SacnIndex(_rangePos);
  return b ? TaskStatus::FINISHED : TaskStatus::INTERVAL;
}

int StatementAction::JudgeRange() {
  _rangePos = _stmt->CalcIndexRanges(_indexTree);
  return _rangePos;
}

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
    vctKey.reserve(vctProp[i]._vctCol.size());

    for (IndexColumn &col : vctProp[i]._vctCol) {
      IDataValue *dv = _stmtRecord->_vctParas[col.colPos];
      vctKey.push_back(dv->AddRef());
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
    lrPri->RleaseOverflowPage(vctProp[0]._tree, true);

    for (LeafRecord *lr : vctLr) {
      LeafRecord::FreeRecord(lr, true);
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
    if (_stmtSecRec->_bReleaseLock) {
      _stmtSecRec->_secLr->SubmitStatement(*_stmtSecRec->_stmt,
                                           RecordStatus::FREEED);
    }
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

  ExprType exprType = _stmtSecRec->_stmt->GetType();
  assert(exprType == ExprType::EXPR_UPDATE ||
         exprType == ExprType::EXPR_DELETE ||
         exprType == ExprType::EXPR_TABLE_SELECT);

  TriBool tb = _stmtSecRec->_stmt->HandleLeafRecord(lp, pos, _rangePos,
                                                    &_stmtSecRec->_vctLr);
  if (_stmtSecRec->_bReleaseLock) {
    _stmtSecRec->_secLr->SubmitStatement(*_stmtSecRec->_stmt,
                                         RecordStatus::FREEED);
  }
  if (tb == TriBool::Error) {
    _stmtSecRec->_status.store(ActionStatus::FAILED, memory_order_release);
  } else if (tb == TriBool::False) {
    _stmtSecRec->_status.store(ActionStatus::SUCEED, memory_order_release);
  } else {
    _stmtSecRec->_numLeafRecord = 1;
    _stmtSecRec->_status.store(ActionStatus::SUCEED, memory_order_release);
  }

  lp->AddWriteQueue(_indexTree->GetVctRange()[_rangePos]._pageMap);
  return TaskStatus::FINISHED;
}

int StmtSecRecordAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_stmtSecRec->_secLr->GetPrimayKey());
  return _rangePos;
}

} // namespace storage