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
    return TaskStatus::RUNNING;
  }

  assert(_pageId == _page->GetPageId());
  _page->SetPrevPageId(_prevPageId);
  _page->AddWriteQueue(_indexTree->GetVctRange()[_rangePos]._pageMap);
  return TaskStatus::FINISHED;
}

TaskStatus RecordAction::Exec() {
  if (_idxPage == nullptr) {
    _idxPage = _indexTree->GetRootPage();
  }
  if (_idxPage->GetPageType() != PageType::LEAF_PAGE) {
    bool b = _indexTree->SearchPage(*_lr, _idxPage);
    if (!b) {
      return TaskStatus::RUNNING;
    }
  }

  RecordLock *lock = _lr->GetLock();
  if (lock->GetRecordStatus() != RecordStatus::INIT) {
    assert(lock->GetRecordResult() != RecordResult::IN_PAGE &&
           lock->GetRecordStatus() == RecordStatus::ROLLBACKED);
    delete _lr;
    _lr = nullptr;
    return TaskStatus::FINISHED;
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

  return b ? TaskStatus::FINISHED : TaskStatus::RUNNING;
}

int StatementAction::JudgeRange() { return _stmt->CalcIndexRanges(_indexTree); }

StmtInsertAction::~StmtInsertAction() { delete _stmtRecord; }

TaskStatus StmtInsertAction::Exec() {
  if (_stmtRecord->_stmt->IsStmtFailed()) {
    _stmtRecord->_status.store(ActionStatus::FAILED, memory_order_release);
    return TaskStatus::FINISHED;
  }

  TableTaskMgr *mgr = _stmtRecord->_table->GetTableTaskMgr();
  MVector<storage::IndexProp> &vctProp = _stmtRecord->_table->GetVectorIndex();
  MVector<LeafRecord *> vctLr;

  VersionStamp stamp = vctProp[0]._tree->ApplyStamp(_rangePos);
  LeafRecord *lrPri =
      new LeafRecord(vctProp[0]._tree, _stmtRecord->_priKey,
                     _stmtRecord->_vctParas, stamp, _stmtRecord->_stmt);

  if (!lrPri->IsValid()) {
    _stmtRecord->_status.store(ActionStatus::FAILED, memory_order_release);
    SessionErrMsgAction *eAction = new SessionErrMsgAction(
        _stmtRecord->_stmt, move(_threadErrorMsg->GetErrorMsg()));
    SessionPool::AddAction(ThreadPool::GetThreadId(),
                           _stmtRecord->_stmt->GetTxId(), eAction);
    _stmtRecord->_stmt->SetStmtFailed(true);
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
};

int StmtInsertAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_stmtRecord->_priKey);
  return _rangePos;
};

StmtPriKeyAction::~StmtPriKeyAction() { delete _stmtPriKey; }

TaskStatus StmtPriKeyAction::Exec() { return TaskStatus::UNINIT; };

int StmtPriKeyAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_stmtPriKey->_priKey);
  return _rangePos;
};

} // namespace storage