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
int PrevPageAction::JudgeRange() { return -1; }

TaskStatus PrevPageAction::Exec() {
  assert(_rangePos >= 0 && _rangePos < _indexTree->GetVctRange().size());
  if (_idxPage == nullptr) {
    if (_indexTree->GetVctRange().size() == 1) {
      _idxPage = _indexTree->GetRootPage();
    } else {
      _idxPage = _indexTree->GetVctRange()[_rangePos]._vctRangePage[0];
    }
  }

  while (true) {
    PageStatus ps = _idxPage->GetPageStatus();
    if (ps == PageStatus::READING) {
      return TaskStatus::RUNNING;
    }

    if (ps == PageStatus::READED) {
      _idxPage->SetPageStatus(PageStatus::VALID, true);
    }

    if (_idxPage->GetPageType() == PageType::LEAF_PAGE) {
      LeafPage *lp = (LeafPage *)_idxPage;
      assert(lp->GetPageId() == _pageId);
      lp->SetPrevPageId(_prevPageId);
      return TaskStatus::FINISHED;
    } else {
      BranchPage *bp = (BranchPage *)_idxPage;
      _idxPage = bp->GetChild(0);
      if (_idxPage == nullptr) {
        BranchRecord &br = bp->GetRecord(0, false);
        PageType type = bp->GetPageLevel() == 1 ? PageType::LEAF_PAGE
                                                : PageType::BRANCH_PAGE;
        _idxPage = _indexTree->GetPage(br.GetChildPageId(), type, bp);
      }
    }
  }
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
  }

  _lr = nullptr;
  return TaskStatus::FINISHED;
}

int RecordAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(*_lr);
  return _rangePos;
}

TaskStatus PriKeyAction::Exec() { return TaskStatus::FINISHED; }

int PriKeyAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_key);
  return _rangePos;
}

TaskStatus StatementAction::Exec() { return TaskStatus::FINISHED; }

int StatementAction::JudgeRange() { return _stmt->CalcIndexRange(_indexTree); }

InsertAction::InsertAction(PhysTable *table, RawKey &&priKey,
                           VectorDataValue &&recValue, InsertStatement *stmt)
    : IndexAction(table->GetVectorIndex()[0]._tree), _table(table),
      _priKey(move(priKey)), _recValue(move(recValue)), _stmt(stmt) {}

TaskStatus InsertAction::Exec() {
  if (_idxPage == nullptr) {
    MVector<IndexProp> &vctIndex = _table->GetVectorIndex();
    VersionStamp stamp = vctIndex[0]._tree->ApplyStamp(_rangePos);

    _priLr = new LeafRecord(_indexTree, _priKey, _recValue, stamp, _stmt,
                            _indexTree->IsMultiRange());
    if (!_priLr->IsValid()) {
      _indexTree->GetVctRange()[_rangePos]._vctErrRecord.push_back(_priLr);
      return TaskStatus::FINISHED;
    }

    // SessionPool::AddAction(_stmt->GetTxId(), ThreadPool::GetThreadId(),
    // _priLr);
    TableTaskMgr *mgr = _table->GetTableTaskMgr();

    for (size_t i = 1; i < _table->GetVectorIndex().size(); i++) {
      IndexProp &idxProp = _table->GetVectorIndex()[i];
      VectorDataValue vctKey;
      vctKey._bDec = false;
      vctKey.reserve(idxProp._vctCol.size());
      for (IndexColumn &col : idxProp._vctCol) {
        IDataValue *dv = _recValue[col.colPos];
        vctKey.push_back(dv);
      }

      LeafRecord *lrSec = new LeafRecord(
          idxProp._tree, vctKey, _priLr->GetBysValue() + UI16_2_LEN,
          _priLr->GetKeyLength(), ActionType::INSERT, stamp, _stmt);
      if (!lrSec->IsValid()) {
        _indexTree->GetVctRange()[_rangePos]._vctErrRecord.push_back(_priLr);
        return TaskStatus::FINISHED;
      }

      RecordAction *action = new RecordAction(idxProp._tree, lrSec);
      mgr->AddFromPrimaryAction((uint16_t)i, action->JudgeRange(), action);
      // SessionPool::AddAction(ThreadPool::GetThreadId(), _stmt->GetTxId(),
      //                        action);
    }

    _idxPage = _indexTree->GetRootPage();
  }

  if (_idxPage->GetPageType() != PageType::LEAF_PAGE) {
    bool b = _indexTree->SearchPage(*_priLr, _idxPage);
    if (!b) {
      return TaskStatus::RUNNING;
    }
  }

  RecordLock *lock = _priLr->GetLock();
  if (lock->GetRecordStatus() != RecordStatus::INIT) {
    assert(lock->GetRecordResult() != RecordResult::IN_PAGE &&
           lock->GetRecordStatus() == RecordStatus::ROLLBACKED);
    delete _priLr;
    _priLr = nullptr;
    return TaskStatus::FINISHED;
  }

  LeafPage *lp = (LeafPage *)_idxPage;
  lp->UpdateAction(_priLr);
  _priLr = nullptr;
  return TaskStatus::FINISHED;
}

int InsertAction::JudgeRange() {
  _rangePos = _indexTree->CalcIndexRange(_priKey);
  return _rangePos;
}
} // namespace storage